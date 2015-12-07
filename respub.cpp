/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


// libircbot irc::bot::
#include "ircbot/bot.h"

// SPQF
using namespace irc::bot;
#include "log.h"
#include "logs.h"
#include "vote.h"
#include "votes.h"
#include "vdb.h"
#include "praetor.h"
#include "voting.h"
#include "respub.h"


const ResPublica *respub;


extern "C" __attribute__((constructor))
void module_ctor()
noexcept
{
    printf("Adding RexPublica...\n");
}


extern "C" __attribute__((destructor))
void module_dtor()
noexcept
{
	printf("Removing RexPublica...\n");
}


extern "C"
void module_init(Bot *const bot)
noexcept
{
	const std::lock_guard<Bot> lock(*bot);
	printf("Construct RexPublica...\n");
	respub = new ResPublica(*bot);
}


extern "C"
void module_fini(Bot *const bot)
noexcept
{
	const std::lock_guard<Bot> lock(*bot);
	printf("Destruct RexPublica...\n");
	delete respub;
}


ResPublica::ResPublica(Bot &bot):
bot(bot),
opts(bot.opts),
sess(bot.sess),
users(bot.users),
chans(bot.chans),
events(bot.events),
logs(sess,chans,users),
vdb({opts["dbdir"] + "/vote"}),
praetor(sess,chans,users,bot,vdb,logs),
voting(sess,chans,users,logs,bot,vdb,praetor)
{
	// Channel->User catch-all for logging
	events.chan_user.add(handler::ALL,boost::bind(&Logs::log,&logs,_1,_2,_3),handler::RECURRING);

	// Channel command handlers
	events.chan_user.add("PRIVMSG",boost::bind(&ResPublica::handle_privmsg,this,_1,_2,_3),handler::RECURRING);
	events.chan_user.add("NOTICE",boost::bind(&ResPublica::handle_notice,this,_1,_2,_3),handler::RECURRING);
	events.chan.add("MODE",boost::bind(&ResPublica::handle_cmode,this,_1,_2),handler::RECURRING);

	// Private command handlers
	events.user.add("PRIVMSG",boost::bind(&ResPublica::handle_privmsg,this,_1,_2),handler::RECURRING);
	events.user.add("NOTICE",boost::bind(&ResPublica::handle_notice,this,_1,_2),handler::RECURRING);

	// Misc handlers
	events.user.add("NICK",boost::bind(&ResPublica::handle_nick,this,_1,_2),handler::RECURRING);
	events.chan.add(LIBIRC_RFC_ERR_CHANOPRIVSNEEDED,boost::bind(&ResPublica::handle_not_op,this,_1,_2),handler::RECURRING);
	events.chan_user.add(LIBIRC_RFC_ERR_USERONCHANNEL,boost::bind(&ResPublica::handle_on_chan,this,_1,_2,_3),handler::RECURRING);
	events.chan.add(/* RPL_KNOCK */ 710,boost::bind(&ResPublica::handle_knock,this,_1,_2),handler::RECURRING);
	events.chan.add(/* ERR_MODEISLOCKED */ 742,boost::bind(&ResPublica::handle_mlock,this,_1,_2),handler::RECURRING);
}


ResPublica::~ResPublica()
noexcept
{
	events.chan_user.clear(handler::Prio::USER);
	events.user.clear(handler::Prio::USER);
	events.chan.clear(handler::Prio::USER);
}


///////////////////////////////////////////////////////////////////////////////////
//
//   Primary dispatch (irc::bot::Bot overloads)
//


void ResPublica::handle_privmsg(const Msg &msg,
                                Chan &chan,
                                User &user)
try
{
	using namespace fmt::PRIVMSG;

	// Discard empty without exception
	if(msg[TEXT].empty())
		return;

	// Silently drop the background noise
	if(!user.is_logged_in())
		return;

	// Notify active votes listening to channels
	voting.for_each([&](Vote &vote)
	{
		vote.event_chanmsg(user,chan,msg[TEXT]);

		if(chan == vote.get_chan_name())
			vote.event_chanmsg(user,msg[TEXT]);
	});

	// Discard everything not starting with command prefix
	if(msg[TEXT].find(opts["prefix"]) != 0)
		return;

	handle_cmd(msg,chan,user);
}
catch(const Assertive &e)
{
	user << chan << "Internal Error: " << e << flush;
	throw;
}
catch(const Exception &e)
{
	user << chan << "Failed: " << e << flush;
}
catch(const std::out_of_range &e)
{
	user << chan << "You did not supply required arguments. Use the help command." << flush;
}


void ResPublica::handle_notice(const Msg &msg,
                               Chan &chan,
                               User &user)
try
{
	using namespace fmt::NOTICE;

	// Discard empty without exception
	if(msg[TEXT].empty())
		return;

	// Silently drop the background noise
	if(!user.is_logged_in())
		return;

	// Notify active votes listening to channels
	voting.for_each([&](Vote &vote)
	{
		vote.event_cnotice(user,chan,msg[TEXT]);

		if(chan == vote.get_chan_name())
			vote.event_cnotice(user,msg[TEXT]);
	});
}
catch(const Assertive &e)
{
	user << chan << "Internal Error: " << e << flush;
	throw;
}
catch(const Exception &e)
{
	user << chan << "Failed: " << e << flush;
}
catch(const std::out_of_range &e)
{
	user << chan << "You did not supply required arguments. Use the help command." << flush;
}


void ResPublica::handle_cmode(const Msg &msg,
                              Chan &chan)
{
	using namespace fmt::MODE;

	if(msg.from("chanserv") || msg.from(sess.get_nick()))
		return;

	const auto &serv(sess.get_server());
	User &user(users.get(msg.get_nick()));
	const Deltas deltas(detok(msg.begin()+1,msg.end()),sess.get_server());
	for(const auto &delta : deltas)
	{
		if(bool(delta) && (delta == 'q' || delta == 'b'))
		{
			voting.motion<vote::Appeal>(chan,user,std::string(~delta));
			chan << user.get_nick() << "'s unilateral executive decision invoked this automatic appeal process." << flush;
		}
	}
}


void ResPublica::handle_privmsg(const Msg &msg,
                                User &user)
try
{
	using namespace fmt::PRIVMSG;

	// Discard empty without exception
	if(msg[TEXT].empty())
		return;

	// Silently drop the background noise
	if(!user.is_logged_in())
		return;

	// Notify active votes listening for private messages
	voting.for_each([&](Vote &vote)
	{
		vote.event_privmsg(user,msg[TEXT]);
	});

	handle_cmd(msg,user);
}
catch(const Assertive &e)
{
	user << "Internal Error: " << e << flush;
	throw;
}
catch(const Exception &e)
{
	user << "Failed: " << e << flush;
}
catch(const std::out_of_range &e)
{
	user << "You did not supply required arguments. Use the help command." << flush;
}


void ResPublica::handle_notice(const Msg &msg,
                               User &user)
try
{
	using namespace fmt::NOTICE;

	// Discard empty without exception
	if(msg[TEXT].empty())
		return;

	// Silently drop the background noise
	if(!user.is_logged_in())
		return;

	// Notify active votes listening for private messages
	voting.for_each([&](Vote &vote)
	{
		vote.event_notice(user,msg[TEXT]);
	});
}
catch(const Assertive &e)
{
	user << "Internal Error: " << e << flush;
	throw;
}
catch(const Exception &e)
{
	user << "Failed: " << e << flush;
}
catch(const std::out_of_range &e)
{
	user << "You did not supply required arguments. Use the help command." << flush;
}


void ResPublica::handle_nick(const Msg &msg,
                             User &user)
{
	const auto &old_nick = msg.get_nick();
	const auto &new_nick = user.get_nick();

	voting.for_each([&](Vote &vote)
	{
		vote.event_nick(user,old_nick);
	});
}


void ResPublica::handle_not_op(const Msg &msg,
                               Chan &chan)
{
	using namespace fmt::CHANOPRIVSNEEDED;

	chan << "I'm afraid I can't do that. (" << msg[REASON] << ")" << chan.flush;
}


void ResPublica::handle_on_chan(const Msg &msg,
                                Chan &chan,
                                User &user)
{
	using namespace fmt::USERONCHANNEL;

	chan << "It seems " << msg[NICKNAME] << " " << msg[REASON] << "." << chan.flush;
}


void ResPublica::handle_knock(const Msg &msg,
                              Chan &chan)
{
	using namespace fmt::KNOCK;

	chan << "It seems " << msg[MASK] << " " << msg[REASON] << "." << chan.flush;
}


void ResPublica::handle_mlock(const Msg &msg,
                              Chan &chan)
{
    using namespace fmt::MODEISLOCKED;

	chan << "I'm sorry " << msg[CHANNAME] << ", I'm afraid I can't do that. '"
	     << msg[MODEUSED] << "' " << msg[REASON] << "."
	     << chan.flush;
}


///////////////////////////////////////////////////////////////////////////////////
//
//   Channel message handlers
//

void ResPublica::handle_cmd(const Msg &msg,
                            Chan &chan,
                            User &user)
{
	using namespace fmt::PRIVMSG;

	// Chop off cmd prefix and dispatch
	const auto tok(tokens(msg[TEXT]));
	switch(hash(tok.at(0).substr(opts["prefix"].size())))
	{
		case hash("v"):
		case hash("vote"):     handle_vote(msg,chan,user,subtok(tok));    break;
		case hash("version"):  handle_version(msg,chan,user,subtok(tok)); break;
		default:                                                          break;
	}
}



void ResPublica::handle_version(const Msg &msg,
                                Chan &chan,
                                User &user,
                                const Tokens &toks)
{
	user << SPQF_VERSION << flush;
}



void ResPublica::handle_vote(const Msg &msg,
                             Chan &chan,
                             User &user,
                             Tokens toks)
{
	// For UI ease-of-use, when a user ought to be typing !vote yes $id
	// allow them to reverse the syntax
	if(toks.size() == 2 && isnumeric(*toks.at(0)) && Vote::is_ballot(*toks.at(1)))
		std::reverse(toks.begin(),toks.end());

	const std::string &subcmd(!toks.empty()? *toks.at(0) : "help");

	// Handle pattern for accessing vote by ID
	if(isnumeric(subcmd))
	{
		handle_vote_id(msg,chan,user,toks);
		return;
	}

	// Handle pattern for voting on config
	if(subcmd.find("config") == 0)
	{
		handle_vote_config(msg,chan,user,toks);
		return;
	}

	// Handle pattern for voting on modes directly
	if(subcmd.size() > 1 && (subcmd.at(0) == '+' || (subcmd.at(0) == '-' && subcmd.at(1) != '-')))
	{
		voting.motion<vote::Mode>(chan,user,detok(toks));
		return;
	}

	if(Vote::is_ballot(subcmd))
	{
		handle_vote_ballot(msg,chan,user,subtok(toks),Vote::ballot(subcmd));
		return;
	}

	// Handle various sub-keywords
	switch(hash(subcmd))
	{
		// Administrative
		default:
		case hash("help"):     handle_help(msg,user<<chan,subtok(toks));                     break;
		case hash("count"):
		case hash("list"):     handle_vote_list(msg,chan,user,subtok(toks));                 break;
		case hash("info"):     handle_vote_id(msg,chan,user,subtok(toks));                   break;
		case hash("cancel"):   handle_vote_cancel(msg,chan,user,subtok(toks));               break;
		case hash("eligible"): handle_vote_eligible(msg,chan,user,subtok(toks));             break;

		// Actual vote types
		case hash("ban"):      voting.motion<vote::Ban>(chan,user,detok(subtok(toks)));      break;
		case hash("kick"):     voting.motion<vote::Kick>(chan,user,detok(subtok(toks)));     break;
		case hash("mode"):     voting.motion<vote::Mode>(chan,user,detok(subtok(toks)));     break;
		case hash("quiet"):    voting.motion<vote::Quiet>(chan,user,detok(subtok(toks)));    break;
		case hash("unquiet"):  voting.motion<vote::UnQuiet>(chan,user,detok(subtok(toks)));  break;
		case hash("voice"):    voting.motion<vote::Voice>(chan,user,detok(subtok(toks)));    break;
		case hash("devoice"):  voting.motion<vote::DeVoice>(chan,user,detok(subtok(toks)));  break;
		case hash("flags"):    voting.motion<vote::Flags>(chan,user,detok(subtok(toks)));    break;
		case hash("civis"):    voting.motion<vote::Civis>(chan,user,detok(subtok(toks)));    break;
		case hash("censure"):  voting.motion<vote::Censure>(chan,user,detok(subtok(toks)));  break;
		case hash("invite"):   voting.motion<vote::Invite>(chan,user,detok(subtok(toks)));   break;
		case hash("topic"):    voting.motion<vote::Topic>(chan,user,detok(subtok(toks)));    break;
		case hash("opine"):    voting.motion<vote::Opine>(chan,user,detok(subtok(toks)));    break;
		case hash("quote"):    voting.motion<vote::Quote>(chan,user,detok(subtok(toks)));    break;
		case hash("import"):   voting.motion<vote::Import>(chan,user,detok(subtok(toks)));   break;
	}
}


void ResPublica::handle_vote_id(const Msg &msg,
                                Chan &chan,
                                User &user,
                                const Tokens &toks)
try
{
	const Chan *const c(chan.is_op()? &chan : chans.find_cnotice(user));
	const auto id(lex_cast<Vote::id_t>(*toks.at(0)));

	if(c)
	{
		const Vote &vote(voting.exists(id)? voting.get(id) : vdb.get<Vote>(id));
		handle_vote_info(msg,user,user<<(*c),subtok(toks),vote);
	}
	else handle_vote_list(msg,user,user,subtok(toks),id);
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("You supplied a bad ID number.");
}


void ResPublica::handle_vote_cancel(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks)
{
	auto &vote = !toks.empty()? voting.get(lex_cast<Vote::id_t>(*toks.at(0))):
	                            voting.get(chan);
	voting.cancel(vote,chan,user);
}


void ResPublica::handle_vote_ballot(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks,
                                    const Vote::Ballot &ballot)
try
{
	auto &vote = !toks.empty()? voting.get(lex_cast<Vote::id_t>(*toks.at(0))):
	                            voting.get(chan);
	vote.event_vote(user,ballot);
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("You must supply the vote ID as a number.");
}


void ResPublica::handle_vote_list(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	const Tokens subtoks(subtok(toks));
	if(toks.empty())
	{
		if(!voting.exists(chan))
		{
			const Chan *const &cmsg_chan(chans.find_cnotice(user));
			user << cmsg_chan << "No active votes for this channel." << user.flush;
		}
		else voting.for_each(chan,[this,&msg,&chan,&user,&subtoks]
		(const Vote &vote)
		{
			const auto &id(vote.get_id());
			handle_vote_list(msg,chan,user,subtoks,id);
		});
	} else {
		const auto &id(lex_cast<Vote::id_t>(*toks.at(0)));
		handle_vote_list(msg,chan,user,subtoks,id);
	}
}


void ResPublica::handle_vote_list(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks,
                                  const id_t &id)
{
	if(chan.get("config.vote.list")["ack.chan"] == "1")
		handle_vote_list(msg,user,chan,toks,id);
	else
		handle_vote_list(msg,user,user<<chan,toks,id);
}


void ResPublica::handle_vote_config(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks)
{
	const std::string issue = detok(toks);
	if(issue.find("=") != std::string::npos)
	{
		voting.motion<vote::Config>(chan,user,issue);
		return;
	}

	const Adoc &cfg = chan.get();
	const std::string &key = *toks.at(0);
	const std::string &val = cfg[key];

	const bool ack_chan = cfg["config.config.ack.chan"] == "1";
	Locutor &out = ack_chan? static_cast<Locutor &>(chan):
	                         static_cast<Locutor &>(user << chan);   // CMSG
	if(val.empty())
	{
		const Adoc &doc = cfg.get_child(key,Adoc());
		out << doc << flush;
	}
	else out << key << " = " << val << flush;
}


void ResPublica::handle_vote_eligible(const Msg &msg,
                                      Chan &chan,
                                      User &user,
                                      const Tokens &toks)
try
{
	if(!chan.users.mode(user).has('o'))
	{
		user << "You must be an operator to run this command" << flush;
		return;
	}

	std::map<std::string,uint> count;
	std::map<std::string,std::string> accts;

	const Adoc &cfg(chan.get("config.vote"));
	const auto lines(cfg.get<uint>("civis.eligible.lines"));
	const auto age(secs_cast(cfg["civis.eligible.age"]));

	chan << "Just a moment while I find users with >" << lines << " lines "
	     << "before " << secs_cast(age) << " ago."
	     << flush;

	Logs::SimpleFilter filt;
	filt.type = "PRI";  // PRIVMSG
	filt.time.first = 0;
	filt.time.second = time(NULL) - age;
	logs.for_each(chan.get_name(),filt,[&count,&accts]
	(const Logs::ClosureArgs &a)
	{
		if(strlen(a.acct) == 0 || *a.acct == '*')
			return true;

		++count[a.acct];
		const auto iit(accts.emplace(a.acct,a.nick));
		if(iit.second)
			return true;

		auto &nick(iit.first->second);
		if(nick != a.nick)
			nick = a.nick;

		return true;
	});

	std::stringstream str;
	for(const auto &p : count) try
	{
		const auto &acct(p.first);
		const auto &linecnt(p.second);
		if(linecnt < lines)
			continue;

		const auto &nick(accts.at(acct));
		const User user(&bot.adb,&bot.sess,&bot.ns,nick,"",acct);
		if(chan.lists.has_flag(user,'V'))
			continue;

		str << nick << " (" << acct << " : " << linecnt << ") ";
	}
	catch(const std::out_of_range &e)
	{
		printf("error on %s (%u)\n",p.first.c_str(),p.second);
		continue;
	}

	chan << user.get_nick() << ", results: " << str.str() << flush;
}
catch(const std::out_of_range &e)
{
	throw Exception("Need a channel name because this is PM.");
}


///////////////////////////////////////////////////////////////////////////////////
//
//   Private message handlers
//

void ResPublica::handle_cmd(const Msg &msg,
                            User &user)
{
	using namespace fmt::PRIVMSG;

	// Strip off the command prefix if given
	const auto toks = tokens(msg[TEXT]);
	const bool has_prefix = toks.at(0).find(opts["prefix"]) != std::string::npos;
	const auto cmd = has_prefix? toks.at(0).substr(opts["prefix"].size()) : toks.at(0);

	switch(hash(cmd))
	{
		case hash("v"):
		case hash("vote"):     handle_vote(msg,user,subtok(toks));     break;
		case hash("version"):  handle_version(msg,user,subtok(toks));  break;
		case hash("help"):     handle_help(msg,user,subtok(toks));     break;
		case hash("config"):   handle_config(msg,user,subtok(toks));   break;
		case hash("whoami"):   handle_whoami(msg,user,subtok(toks));   break;
		case hash("regroup"):  handle_regroup(msg,user,subtok(toks));  break;
		case hash("praetor"):  handle_praetor(msg,user,subtok(toks));  break;
		default:                                                       break;
	}
}


void ResPublica::handle_config(const Msg &msg,
                               User &user,
                               const Tokens &toks)
try
{
	Chan &chan(chans.get(*toks.at(0)));

	// Founder config override to fix a broken config
	if(toks.size() >= 3 && *toks.at(2) == "=" && chan.lists.has_flag(user,'F'))
	{
		Adoc cfg(chan.get());
		const auto &key(*toks.at(1));

		if(toks.size() == 3)
		{
			if(!cfg.remove(key))
				throw Exception("Failed to find and remove anything with key.");
		} else {
			const auto &val(*toks.at(3));
			cfg.put(key,val);
		}

		chan.set(cfg);
		user << "[FOUNDER OVERRIDE] Success." << user.flush;
		return;
	}

	const Adoc &cfg = chan.get();
	const std::string &key = toks.size() > 1? *toks.at(1) : "";
	const std::string &val = cfg[key];

	if(val.empty())
	{
		const Adoc &doc = cfg.get_child(key,Adoc());
		user << doc << flush;
	}
	else user << key << " = " << val << flush;
}
catch(const std::out_of_range &e)
{
	throw Exception("Need a channel name because this is PM.");
}


void ResPublica::handle_whoami(const Msg &msg,
                               User &user,
                               const Tokens &toks)
{
	user << user.get() << flush;
}


void ResPublica::handle_regroup(const Msg &msg,
                                User &user,
                                const Tokens &toks)
{
	static const time_t limit = 600;
	if(user.get_val<time_t>("info._fetched_") > time(NULL) - limit)
		throw Exception("You've done this too much. Try again later.");

	user.info();
}


void ResPublica::handle_praetor(const Msg &msg,
                                User &user,
                                const Tokens &toks)
{


}


void ResPublica::handle_version(const Msg &msg,
                                User &user,
                                const Tokens &toks)
{
	user << SPQF_VERSION << flush;
}



void ResPublica::handle_vote(const Msg &msg,
                             User &user,
                             Tokens toks)
{
	// For UI ease-of-use, when a user ought to be typing !vote yes $id
	// allow them to reverse the syntax
	if(toks.size() == 2 && isnumeric(*toks.at(0)) && Vote::is_ballot(*toks.at(1)))
		std::reverse(toks.begin(),toks.end());

	const std::string subcmd = !toks.empty()? *toks.at(0) : "help";

	// Handle pattern for accessing vote by ID
	if(isnumeric(subcmd))
	{
		handle_vote_id(msg,user,toks);
		return;
	}

	if(Vote::is_ballot(subcmd))
	{
		handle_vote_ballot(msg,user,subtok(toks),Vote::ballot(subcmd));
		return;
	}

	switch(hash(subcmd))
	{
		case hash("count"):
		case hash("list"):     handle_vote_list(msg,user,subtok(toks));                break;
		case hash("info"):     handle_vote_id(msg,user,subtok(toks));                  break;
		default:
		case hash("help"):     handle_help(msg,user,subtok(toks));                     break;
	}
}


void ResPublica::handle_vote_id(const Msg &msg,
                                User &user,
                                const Tokens &toks)
try
{
	Chan *const chan(chans.find_cnotice(user));
	const auto id(lex_cast<Vote::id_t>(*toks.at(0)));
	if(!chan)
	{
		handle_vote_list(msg,user,user,subtok(toks),id);
		return;
	}

	const Vote &vote(voting.exists(id)? voting.get(id) : vdb.get<Vote>(id));
	handle_vote_info(msg,user,user<<(*chan),subtok(toks),vote);
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("You supplied a bad ID number.");
}


void ResPublica::handle_vote_list(const Msg &msg,
                                  User &user,
                                  const Tokens &toks)
try
{
	auto &chan(chans.get(*toks.at(0)));
	const auto *const cmsg_chan(chans.find_cnotice(user));
	const auto subtoks(subtok(toks));

	if(subtoks.empty())
	{
		if(!voting.exists(chan))
		{
			user << cmsg_chan << "No active votes for this channel." << user.flush;
			return;
		}

		voting.for_each(chan,[this,&msg,&user,&subtoks,&cmsg_chan]
		(const Vote &vote)
		{
			const auto &id(vote.get_id());
			handle_vote_list(msg,user,(user<<cmsg_chan),subtoks,id);
		});

		return;
	}

	if(isnumeric(*subtoks.at(0)))
	{
		const auto &id(lex_cast<Vote::id_t>(*subtoks.at(0)));
		handle_vote_list(msg,user,user,subtoks,id);
		return;
	}

	static const std::map<std::string,std::string> aliases
	{
		{ "speaker", "nick"      },
		{ "channel", "chan"      },
	};

	std::map<std::string,std::string> options
	{
		{ "limit", "10"          },
		{ "order", "descending"  },
	};

	std::forward_list<Vdb::Term> terms
	{
		{ "chan", "##politics" },
	};

	parse_args(detok(subtoks),"--","="," ",[&terms,&options]
	(std::pair<std::string,std::string> kv)
	mutable
	{
		auto &key(kv.first);
		auto &val(kv.second);
		key = tolower(key);
		val = tolower(val);

		if(key.empty() || val.empty())
			throw Exception("Usage: !vote list <#channel> <--key=value> [--key=value]");

		if(!isalpha(key))
			throw Exception("Search keys must contain alpha characters only");

		if(aliases.count(key))
			key = aliases.at(key);

		if(options.count(key))
			options.at(key) = val;
		else
			terms.emplace_front(kv);
	});

	auto res(vdb.find(terms));
	res.sort();

	if(options.at("order") == "descending")
		std::reverse(res.begin(),res.end());

	const size_t max_limit(cmsg_chan? 100 : 10);
	const auto limit(lex_cast<size_t>(options.at("limit")));
	res.resize(std::min(res.size(),limit));

	if(limit > max_limit)
		throw Exception("Exceeded the maximum --limit=") << max_limit;

	if(!res.empty())
		for(const auto &id : res)
			handle_vote_list(msg,user,(user<<cmsg_chan),{},id);
	else
		user << cmsg_chan << "No matching results." << user.flush;
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("You supplied a bad ID number.");
}
catch(const std::out_of_range &e)
{
	throw Exception("Need a channel name because this is PM.");
}


void ResPublica::handle_vote_ballot(const Msg &msg,
                                    User &user,
                                    const Tokens &toks,
                                    const Vote::Ballot &ballot)
try
{
	const auto id = lex_cast<Vote::id_t>(*toks.at(0));
	auto &vote = voting.get(id);
	vote.event_vote(user,ballot);
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("You must supply the vote ID as a number.");
}
catch(const std::out_of_range &e)
{
	throw Exception("You must supply the vote ID given in the channel.");
}


///////////////////////////////////////////////////////////////////////////////////
//
//   Abstract handlers
//

void ResPublica::handle_help(const Msg &msg,
                             Locutor &out,
                             const Tokens &toks)
{
	const std::string topic = !toks.empty()? *toks.at(0) : "";
	switch(hash(topic))
	{
		case hash("config"):  out << "https://github.com/jevolk/SPQF/blob/master/help/config";  break;
		case hash("mode"):    out << "https://github.com/jevolk/SPQF/blob/master/help/mode";    break;
		case hash("kick"):    out << "https://github.com/jevolk/SPQF/blob/master/help/kick";    break;
		default:
		case hash("vote"):    out << "https://github.com/jevolk/SPQF/blob/master/help/vote";    break;
	}

	out << flush;
}


void ResPublica::handle_vote_list(const Msg &msg,
                                  const User &user,
                                  Locutor &out,
                                  const Tokens &toks,
                                  const id_t &id)
{
	using namespace colors;

	const Vote &vote(voting.exists(id)? voting.get(id) : vdb.get<Vote>(id));
	const auto cfg(vote.get_cfg());
	const auto tally(vote.tally());
	const scope f([&]
	{
		// Flush may erase the CNOTICE privilege of this stream so it is only done once
		out << flush;
	});

	out << vote << ": ";
	out << BOLD << "YEA" << OFF << ": " << BOLD << FG::GREEN << tally.first << OFF << " ";
	out << BOLD << "NAY" << OFF << ": " << BOLD << FG::RED << tally.second << OFF << " ";
	out << BOLD << "YOU" << OFF << ": ";

	if(!vote.voted(user))
		out << BOLD << FG::BLACK << "---" << OFF << " ";
	else if(vote.position(user) == Vote::YEA)
		out << BOLD << FG::WHITE << BG::GREEN << "YEA" << OFF << " ";
	else if(vote.position(user) == Vote::NAY)
		out << BOLD << FG::WHITE << BG::RED << "NAY" << OFF << " ";

	if(vote.remaining() < 0 && vote.get_reason().empty())
		out << BOLD << UNDER2 << FG::WHITE << BG::GREEN << "PASSED" << OFF << " ";
	else if(vote.remaining() >= 0 && vote.get_reason().empty())
		out << BOLD << FG::WHITE << BG::ORANGE << "ACTIVE" << OFF << " ";
	else
		out << BOLD << UNDER2 << FG::WHITE << BG::RED << "FAILED" << OFF << " ";

	out << "| " << BOLD << vote.get_type() << OFF << ": " << UNDER2 << vote.get_issue() << OFF << " - ";

	if(vote.remaining() >= 0 && vote.get_reason().empty())
	{
		const auto total(vote.total());
		const auto required(vote.required());
		const auto quorum(vote.get_quorum());

		if(cfg.get<time_t>("for") > 0)
			out << "For " << BOLD << secs_cast(cfg.get<time_t>("for")) << OFF << ". ";

		out << "There are " << BOLD << secs_cast(vote.remaining()) << OFF << " left. ";

		if(total < quorum)
			out << BOLD << (quorum - total) << OFF << " more votes are required for a quorum.";
		else if(tally.first < required)
			out << BOLD << (required - tally.first) << OFF << " more yeas are required to pass.";
		else
			out << "As it stands, the motion will pass.";
	} else
	{
		const auto ago(time(nullptr) - vote.get_ended());
		const auto eff(vote.expires() - time(nullptr));

		if(!vote.get_reason().empty())
			out << BOLD << FG::RED << vote.get_reason() << OFF << ". ";
		else if(cfg.get<time_t>("for") > 0 && eff > 0)
			out << BOLD << FG::GREEN << "effective " << OFF << secs_cast(eff) << " more. ";

		out << secs_cast(ago) << " ago.";
	}
}


void ResPublica::handle_vote_info(const Msg &msg,
                                  const User &user,
                                  Locutor &out,
                                  const Tokens &toks,
                                  const Vote &vote)
{
	using namespace colors;

	const std::string pfx(std::string("#") + string(vote.get_id()) + ": ");
	const auto &cfg(vote.get_cfg());
	const auto tally(vote.tally());
	const scope f([&]
	{
		// Flush may erase the CNOTICE privilege of this stream so it is only done once
		out << flush;
	});

	// Title line
	out << pfx << "Information on vote " << vote << ": ";

	if(!vote.get_ended())
		out << BOLD << FG::GREEN << "ACTIVE" << OFF << " (remaining: " << BOLD << vote.remaining() << OFF << "s)" << "\n";
	else
		out << BOLD << FG::RED << "CLOSED" << "\n";

	// Issue line
	out << pfx << BOLD << "ISSUE" << OFF << "    : " << vote.get_issue() << "\n";

	// Type line
	out << pfx << BOLD << "TYPE" << OFF << "     : " << vote.get_type() << "\n";

	// Chan line
	out << pfx << BOLD << "CHANNEL" << OFF << "  : " << vote.get_chan_name() << "\n";

	// Initiator line
	out << pfx << BOLD << "SPEAKER" << OFF << "  : " << vote.get_user_acct() << "\n";

	// Quorum line
	if(vote.get_quorum() > 0)
		out << pfx << BOLD << "QUORUM" << OFF << "   : " << vote.get_quorum() << "\n";

	// Time lines
	struct tm tm;
	char timestr[64] {0};

	// Started line
	const auto began(vote.get_began());
	gmtime_r(&began,&tm);
	strftime(timestr,sizeof(timestr),"%c",&tm);
	out << pfx << BOLD << "STARTED" << OFF << "  : " << began << " (" << timestr << ")\n";

	// Ended line
	const auto ended(vote.get_ended());
	if(ended)
	{
		gmtime_r(&ended,&tm);
		strftime(timestr,sizeof(timestr),"%c",&tm);
		out << pfx << BOLD << "ENDED" << OFF << "    : " << ended << " (" << timestr << ")\n";
	}

	// Yea votes line
	if(tally.first)
	{
		out << pfx << BOLD << "YEA" << OFF << "      : " << BOLD << FG::GREEN << tally.first << OFF;
		if(cfg["visible.ballots"] == "1")
		{
			out << " - ";
			for(const auto &acct : vote.get_yea())
				out << acct << ", ";
		}
		out << "\n";
	}

	// Nay votes line
	if(tally.second)
	{
		out << pfx << BOLD << "NAY" << OFF << "      : " << BOLD << FG::RED << tally.second << OFF;
		if(cfg["visible.ballots"] == "1")
		{
			out << " - ";
			for(const auto &acct : vote.get_nay())
				out << acct << ", ";
		}
		out << "\n";
	}

	// Veto votes line
	if(vote.num_vetoes())
	{
		out << pfx << BOLD << "VETO" << OFF << "     : " << BOLD << FG::MAGENTA << vote.num_vetoes() << OFF;
		if(cfg["visible.veto"] == "1")
		{
			out << " - ";
			for(const auto &acct : vote.get_veto())
				out << acct << ", ";
		}
		out << "\n";
	}

	// User's position line
	out << pfx << BOLD << "YOU      : " << OFF;
	if(!vote.voted(user))
		out << FG::BLACK << BG::LGRAY << "---" << "\n";
	else if(vote.position(user) == Vote::YEA)
		out << BOLD << FG::WHITE << BG::GREEN << "YEA" << "\n";
	else if(vote.position(user) == Vote::NAY)
		out << BOLD << FG::WHITE << BG::RED << "NAY" << "\n";
	else
		out << FG::BLACK << BG::LGRAY << "???" << "\n";

	// Effect
	if(!vote.get_effect().empty())
		out << pfx << BOLD << "EFFECT" << OFF << "   : " << vote.get_effect() << "\n";

	if(cfg.get<time_t>("for") > 0)
		out << pfx << BOLD << "FOR" << OFF << "      : " << cfg["for"] << " seconds (" << secs_cast(cfg.get<time_t>("for",0)) << ")\n";

	// Result/Status line
	if(vote.get_ended())
	{
		out << pfx << BOLD << "RESULT" << OFF << "   : ";
		if(vote.get_reason().empty())
			out << BOLD << FG::WHITE << BG::GREEN << "PASSED" << "\n";
		else
			out << BOLD << FG::WHITE << BG::RED << "FAILED" << OFF << BOLD << FG::RED << ": " << vote.get_reason() << "\n";
	} else {
		const auto total(vote.total());
		const auto quorum(vote.get_quorum());
		const auto required(vote.required());

		out << pfx << BOLD << "STATUS" << OFF << "   : ";
		if(total < quorum)
			out << BOLD << FG::BLUE << (quorum - total) << " more votes are required for a quorum."  << "\n";
		else if(tally.first < required)
			out << BOLD << FG::ORANGE << (required - tally.first) << " more yeas are required to pass."  << "\n";
		else
			out << FG::GREEN << "As it stands, the motion will pass."  << "\n";
	}
}


///////////////////////////////////////////////////////////////////////////////////
//
//   Utils
//

Tokens subtok(const Tokens &tokens)
{
	return {tokens.begin() + !tokens.empty(), tokens.end()};
}


Tokens subtok(const std::vector<std::string> &tokens)
{
	return pointers<Tokens>(tokens.begin() + !tokens.empty(),tokens.end());
}


std::string detok(const Tokens &tokens)
{
	std::stringstream str;
	for(auto it = tokens.begin(); it != tokens.end(); ++it)
		str << **it << (it == tokens.end() - 1? "" : " ");

	return str.str();
}
