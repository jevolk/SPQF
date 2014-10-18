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
#include "vote.h"
#include "votes.h"
#include "voting.h"
#include "respub.h"


///////////////////////////////////////////////////////////////////////////////////
//
//   Primary dispatch (irc::bot::Bot overloads)
//

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

	handle_cmd(msg,user);
}
catch(const Exception &e)
{
	user << "Failed: " << e << flush;
}
catch(const std::out_of_range &e)
{
	user << "You did not supply required arguments. Use the help command." << flush;
}


void ResPublica::handle_chanmsg(const Msg &msg,
                                Chan &chan,
                                User &user)
try
{
	using namespace fmt::CHANMSG;

	// Discard empty without exception
	if(msg[TEXT].empty())
		return;

	// Silently drop the background noise
	if(!user.is_logged_in())
		return;

	// Discard everything not starting with command prefix
	const Sess &sess = get_sess();
	const Ident &id = sess.get_ident();
	if(msg[TEXT].find(id["prefix"]) != 0)
		return;

	handle_cmd(msg,chan,user);
}
catch(const Exception &e)
{
	user << chan << "Failed: " << e << flush;
}
catch(const std::out_of_range &e)
{
	user << chan << "You did not supply required arguments. Use the help command." << flush;
}


///////////////////////////////////////////////////////////////////////////////////
//
//   Channel message handlers
//

void ResPublica::handle_cmd(const Msg &msg,
                            Chan &chan,
                            User &user)
{
	using namespace fmt::CHANMSG;

	// Chop off cmd prefix and dispatch
	const Sess &sess = get_sess();
	const Ident &id = sess.get_ident();
	const std::vector<std::string> tok = tokens(msg[TEXT]);
	switch(hash(tok.at(0).substr(id["prefix"].size())))
	{
		case hash("vote"):     handle_vote(msg,chan,user,subtok(tok));    break;
		default:                                                          break;
	}
}


void ResPublica::handle_vote(const Msg &msg,
                             Chan &chan,
                             User &user,
                             const Tokens &toks)
{
	const std::string subcmd = !toks.empty()? *toks.at(0) : "help";

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
	if(subcmd.at(0) == '+' || subcmd.at(0) == '-')
	{
		voting.motion<vote::Mode>(chan,user,detok(toks));
		return;
	}

	// Handle various sub-keywords
	switch(hash(subcmd))
	{
		// Ballot
		case hash("yes"):
		case hash("yea"):
		case hash("Y"):
		case hash("y"):        handle_vote_ballot(msg,chan,user,subtok(toks),Vote::YEA);     break;
		case hash("nay"):
		case hash("no"):
		case hash("N"):
		case hash("n"):        handle_vote_ballot(msg,chan,user,subtok(toks),Vote::NAY);     break;

		// Administrative
		default:
		case hash("help"):     handle_help(msg,user,subtok(toks));                           break;
		case hash("count"):
		case hash("list"):     handle_vote_list(msg,chan,user,subtok(toks));                 break;
		case hash("cancel"):   handle_vote_cancel(msg,chan,user,subtok(toks));               break;

		// Actual vote types
		case hash("ban"):      voting.motion<vote::Ban>(chan,user,detok(subtok(toks)));      break;
		case hash("kick"):     voting.motion<vote::Kick>(chan,user,detok(subtok(toks)));     break;
		case hash("mode"):     voting.motion<vote::Mode>(chan,user,detok(subtok(toks)));     break;
		case hash("quiet"):    voting.motion<vote::Quiet>(chan,user,detok(subtok(toks)));    break;
		case hash("unquiet"):  voting.motion<vote::UnQuiet>(chan,user,detok(subtok(toks)));  break;
		case hash("invite"):   voting.motion<vote::Invite>(chan,user,detok(subtok(toks)));   break;
		case hash("topic"):    voting.motion<vote::Topic>(chan,user,detok(subtok(toks)));    break;
		case hash("opine"):    voting.motion<vote::Opine>(chan,user,detok(subtok(toks)));    break;
	}

}


void ResPublica::handle_vote_id(const Msg &msg,
                                Chan &chan,
                                User &user,
                                const Tokens &toks)
{
	const Vote::id_t id = boost::lexical_cast<Vote::id_t>(*toks.at(0));

	if(voting.has_vote(id))
		handle_vote_info(msg,user,user<<chan,subtok(toks),voting.get(id));
	else
		handle_vote_info(msg,user,user<<chan,subtok(toks),{id,get_adb()});
}


void ResPublica::handle_vote_cancel(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks)
{
	auto &vote = !toks.empty()? voting.get(boost::lexical_cast<Vote::id_t>(*toks.at(0))):
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
	auto &vote = !toks.empty()? voting.get(boost::lexical_cast<Vote::id_t>(*toks.at(0))):
	                            voting.get(chan);
	vote.vote(ballot,user);
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("You must supply the vote ID as a number");
}


void ResPublica::handle_vote_list(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	const Tokens subtoks = subtok(toks);

	if(toks.empty())
	{
		voting.for_each(chan,[&]
		(const Vote &vote)
		{
			const auto &id = vote.get_id();
			handle_vote_list(msg,chan,user,subtoks,id);
		});
	} else {
		const auto &id = boost::lexical_cast<Vote::id_t>(*toks.at(0));
		handle_vote_list(msg,chan,user,subtoks,id);
	}
}


void ResPublica::handle_vote_list(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks,
                                  const id_t &id)
{
	if(chan.get("config.vote.list")["ack_chan"] == "1")
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

	const bool ack_chan = cfg["config.config.ack_chan"] == "1";
	Locutor &out = ack_chan? static_cast<Locutor &>(chan):
	                         static_cast<Locutor &>(user);
	if(val.empty())
	{
		const Adoc &doc = cfg.get_child(key,Adoc());
		out << doc << flush;
	}
	else out << key << " = " << val << flush;
}


///////////////////////////////////////////////////////////////////////////////////
//
//   Private message handlers
//

void ResPublica::handle_cmd(const Msg &msg,
                            User &user)
{
	using namespace fmt::PRIVMSG;

	const Sess &sess = get_sess();
	const Ident &id = sess.get_ident();

	// Strip off the command prefix if given
	const std::vector<std::string> toks = tokens(msg[TEXT]);
	const bool has_prefix = toks.at(0).find(id["prefix"]) != std::string::npos;
	const std::string cmd = has_prefix? toks.at(0).substr(id["prefix"].size()) : toks.at(0);

	switch(hash(cmd))
	{
		case hash("vote"):     handle_vote(msg,user,subtok(toks));     break;
		case hash("help"):     handle_help(msg,user,subtok(toks));     break;
		case hash("config"):   handle_config(msg,user,subtok(toks));   break;
		case hash("whoami"):   handle_whoami(msg,user,subtok(toks));   break;
		case hash("regroup"):  handle_regroup(msg,user,subtok(toks));  break;
		default:                                                       break;
	}
}


void ResPublica::handle_config(const Msg &msg,
                               User &user,
                               const Tokens &toks)
try
{
	Chans &chans = get_chans();
	const Chan &chan = chans.get(*toks.at(0));
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


void ResPublica::handle_vote(const Msg &msg,
                             User &user,
                             const Tokens &toks)
{
	const std::string subcmd = !toks.empty()? *toks.at(0) : "help";

	// Handle pattern for accessing vote by ID
	if(isnumeric(subcmd))
	{
		handle_vote_id(msg,user,toks);
		return;
	}

	switch(hash(subcmd))
	{
		// Ballot
		case hash("yea"):
		case hash("yes"):
		case hash("Y"):
		case hash("y"):        handle_vote_ballot(msg,user,subtok(toks),Vote::YEA);    break;
		case hash("nay"):
		case hash("no"):
		case hash("N"):
		case hash("n"):        handle_vote_ballot(msg,user,subtok(toks),Vote::NAY);    break;

		case hash("count"):
		case hash("list"):     handle_vote_list(msg,user,subtok(toks));                break;
		default:
		case hash("help"):     handle_help(msg,user,subtok(toks));                     break;
	}
}


void ResPublica::handle_vote_id(const Msg &msg,
                                User &user,
                                const Tokens &toks)
{
	const Vote::id_t id = boost::lexical_cast<Vote::id_t>(*toks.at(0));

	Chans &chans = get_chans();
	Chan *const chan = chans.find_cnotice(user);
	if(!chan)
	{
		handle_vote_list(msg,user,user,subtok(toks),id);
		return;
	}

	Locutor &out = user << (*chan);  // sets CNOTICE

	if(voting.has_vote(id))
		handle_vote_info(msg,user,out,subtok(toks),voting.get(id));
	else
		handle_vote_info(msg,user,out,subtok(toks),{id,get_adb()});
}


void ResPublica::handle_vote_list(const Msg &msg,
                                  User &user,
                                  const Tokens &toks)
try
{
	Chans &chans = get_chans();
	const Chan &chan = chans.get(*toks.at(0));
	const Tokens subtoks = subtok(toks);

	if(subtoks.empty())
	{
		voting.for_each(chan,[&]
		(const Vote &vote)
		{
			const auto &id = vote.get_id();
			handle_vote_list(msg,user,user,subtoks,id);
		});
	} else {
		const auto &id = boost::lexical_cast<Vote::id_t>(*subtoks.at(0));
		handle_vote_list(msg,user,user,subtoks,id);
	}
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
	const Vote::id_t id = boost::lexical_cast<Vote::id_t>(*toks.at(0));
	auto &vote = voting.get(id);
	vote.vote(ballot,user);
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
                                  User &user,
                                  Locutor out,
                                  const Tokens &toks,
                                  const id_t &id)
{
	using namespace colors;

	const Vote &vote = voting.has_vote(id)? voting.get(id) : Vote(id,get_adb());
	const auto tally = vote.tally();

	out << vote << ": ";
	out << BOLD << "YEA" << OFF << ": " << BOLD << FG::GREEN << tally.first << OFF << " ";
	out << BOLD << "NAY" << OFF << ": " << BOLD << FG::RED << tally.second << OFF << " ";
	out << BOLD << "YOU" << OFF << ": ";

	if(!vote.voted(user))
		out << BOLD << FG::BLACK << "---" << OFF << " ";
	else if(vote.position(user) == Vote::YEA)
		out << BOLD << UNDER2 << FG::WHITE << BG::GREEN << "YEA" << OFF << " ";
	else if(vote.position(user) == Vote::NAY)
		out << BOLD << UNDER2 << FG::WHITE << BG::RED << "NAY" << OFF << " ";

	out << "There are " << BOLD << vote.remaining() << BOLD << " seconds left. ";

	if(vote.total() < vote.minimum())
		out << BOLD << (vote.minimum() - vote.total()) << OFF << " more votes are required. ";
	else if(tally.first < vote.required())
		out << BOLD << (vote.required() - tally.first) << OFF << " more yeas are required to pass. ";
	else
		out << "As it stands, the motion will pass.";

	out << flush;
}


void ResPublica::handle_vote_info(const Msg &msg,
                                  User &user,
                                  Locutor out,
                                  const Tokens &toks,
                                  const Vote &vote)
{
	using namespace colors;

	const std::string pfx = string(vote.get_id()) + ": ";
	const auto cfg = vote.get_cfg();
	const auto tally = vote.tally();

	// Title line
	out << pfx << "Information on vote " << vote << ": ";
	if(vote.remaining() > 0)
		out << BOLD << FG::GREEN << "ACTIVE" << OFF << " (remaining: " << BOLD << vote.remaining() << OFF << "s)" << "\n";
	else
		out << BOLD << FG::RED << "CLOSED" << "\n";

	// Issue line
	out << pfx << "[" << BOLD << vote.get_type() << OFF << "]: " << UNDER2 << vote.get_issue() << "\n";

	// User's position line
	out << pfx << BOLD << "YOU    : " << OFF;
	if(!vote.voted(user))
		out << FG::BLACK << "---" << "\n";
	else if(vote.position(user) == Vote::YEA)
		out << BOLD << FG::WHITE << BG::GREEN << "YEA" << "\n";
	else if(vote.position(user) == Vote::NAY)
		out << BOLD << FG::WHITE << BG::RED << "NAY" << "\n";

	// Yea votes line
	if(tally.first)
	{
		out << pfx << BOLD << "YEA" << OFF << "    : " << BOLD << FG::GREEN << tally.first << OFF;
		if(cfg["visible.ballot"] == "1")
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
		out << pfx << BOLD << "NAY" << OFF << "    : " << BOLD << FG::RED << tally.second << OFF;
		if(cfg["visible.ballot"] == "1")
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
		out << pfx << BOLD << "VETO" << OFF << "   : " << BOLD << FG::MAGENTA << vote.num_vetoes() << OFF << " :: ";
		if(cfg["visible.veto"] == "1")
		{
			out << " - ";
			for(const auto &acct : vote.get_veto())
				out << acct << ", ";
		}
		out << "\n";
	}

	// Result/Status line
	if(vote.remaining() <= 0)
	{
		out << pfx << BOLD << "RESULT" << OFF << " : ";
		if(vote.get_reason().empty())
			out << BOLD << FG::WHITE << BG::GREEN << "PASSED" << "\n";
		else
			out << BOLD << FG::WHITE << BG::RED << "FAILED" << OFF << BOLD << FG::RED << ": " << vote.get_reason() << "\n";
	} else {
		out << pfx << BOLD << "STATUS" << OFF << " : ";
		if(vote.total() < vote.minimum())
			out << BOLD << FG::BLUE << (vote.minimum() - vote.total()) << " more votes are required to reach minimums."  << "\n";
		else if(tally.first < vote.required())
			out << BOLD << FG::ORANGE << (vote.required() - tally.first) << " more votes are required to pass."  << "\n";
		else
			out << FG::GREEN << "As it stands, the motion will pass."  << "\n";
	}

	// Flush erases the CNOTICE privilege of this stream, so we only do it once.
	out << flush;
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

Selection karma_tokens(const Tokens &tokens,
                       const Chan &chan,
                       const std::string &oper)
{
	Selection ret;
	for(auto it = tokens.begin(); it != tokens.end(); ++it)
	{
		const std::string &token = **it;
		if(token.find_last_of(oper) == std::string::npos)
			continue;

		const std::string nick = token.substr(0,token.size()-oper.size());
		if(chan.has_nick(nick))
			ret.push_front(it);
	}

	return ret;
}
