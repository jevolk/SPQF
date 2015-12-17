/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


// libircbot irc::bot::
#include "ircbot/bot.h"
using namespace irc::bot;

// SPQF
#include "log.h"
#include "vote.h"
#include "votes.h"


decltype(vote::names) vote::names
{{
	"config",
	"mode",
	"appeal",
	"trial",
	"kick",
	"invite",
	"topic",
	"opine",
	"quote",
	"ban",
	"unban",
	"quiet",
	"unquiet",
	"voice",
	"devoice",
	"op",
	"deop",
	"exempt",
	"unexempt",
	"flags",
	"import",
	"civis",
	"censure",
	"staff",
	"destaff",
}};



///////////////////////////////////////////////////////////////////////////////
//
// Import - Copy a configuration from another channel
//


void vote::Import::starting()
{
	static const auto usage("!vote import <#chan> <nickname of bot>");

	if(get_target_bot().empty())
		throw Exception("You must hint the nickname of the bot: ") << usage;

	if(get_target_chan().empty())
		throw Exception("You must specify a channel to fetch from: ") << usage;

	if(tolower(get_target_chan()) == tolower(get_chan_name()))
		throw Exception("The source of the config must be a different channel.");

	const auto &sess(get_sess());
	const auto &isup(sess.get_isupport());
	const auto type(get_target_chan().substr(0,1));
	if(!isup.find_in("CHANTYPES",type))
		throw Exception("You must specify a valid channel type for this server.");

	if(tolower(get_target_bot()) == tolower(sess.get_nick()))
	{
		auto &chans(get_chans());
		auto &chan(chans.get(get_target_chan()));
		received << chan.get("config.vote");
		return;
	}

	Locutor bot(get_target_bot());
	bot << bot.PRIVMSG << "config " << get_target_chan() << " config.vote" << bot.flush;
}


void vote::Import::event_notice(User &user,
                                const std::string &text)
{
	if(tolower(user.get_nick()) != tolower(get_target_bot()))
		return;

	received << text;
}


void vote::Import::passed()
{
	if(received.str().empty())
		throw Exception("Failed to fetch any config from the source. No change.");

	auto &chan(get_chan());
	chan.set("config.vote",received.str());

	chan << "Applied new configuration"
	     << " (received " << received.str().size() << " bytes)"
	     << chan.flush;
}



///////////////////////////////////////////////////////////////////////////////
//
// UnQuiet
//


void vote::UnQuiet::effective()
{
	auto &chan(get_chan());
	set_effect(chan.unquiet(user));
}



///////////////////////////////////////////////////////////////////////////////
//
// Quiet
//


void vote::Quiet::starting()
{
	const auto &chan(get_chan());
	const auto &excepts(chan.lists.excepts);
	if(excepts.count(user.mask(Mask::HOST)) && excepts.count(user.mask(Mask::ACCT)))
		throw Exception("User is protected by the [+e]xempt list and is immune.");

	const irc::bot::Mode prots("er");
	if(has_access(chan,user,prots))
		throw Exception("User is protected by +e or +r flags and is immune.");
}


void vote::Quiet::effective()
{
	auto &chan(get_chan());
	set_effect(chan.quiet(user));
}



///////////////////////////////////////////////////////////////////////////////
//
// DeVoice
//


void vote::DeVoice::starting()
{
	auto &chan(get_chan());
	if(!chan.users.mode(user).has('v'))
		throw Exception("User at issue is not voiced");
}


void vote::DeVoice::effective()
{
	auto &chan(get_chan());
	set_effect(chan.devoice(user));
}



///////////////////////////////////////////////////////////////////////////////
//
// Voice
//


void vote::Voice::starting()
{
	auto &chan(get_chan());
	if(chan.users.mode(user).has('v'))
		throw Exception("User at issue already has voice");
}


void vote::Voice::effective()
{
	auto &chan(get_chan());
	set_effect(chan.voice(user));
}



///////////////////////////////////////////////////////////////////////////////
//
// Ban
//


void vote::Ban::starting()
{
	const auto &chan(get_chan());
	const auto &excepts(chan.lists.excepts);
	if(excepts.count(user.mask(Mask::HOST)) && excepts.count(user.mask(Mask::ACCT)))
		throw Exception("User is protected by the [+e]xempt list and is immune.");

	const irc::bot::Mode prots("er");
	if(has_access(chan,user,prots))
		throw Exception("User is protected by the +e or +r flag and is immune.");
}


void vote::Ban::effective()
{
	auto &chan(get_chan());
	set_effect(chan.ban(user));
}


void vote::Ban::passed()
{
	static const std::vector<std::string> msgs
	{
		{ "Terminated with prejudice" },
		{ "And I ain't even mad"      },
	};

	const auto &msg(prejudiced()? msgs.at(0) : msgs.at(1));
	auto &chan(get_chan());
	chan.remove(user,msg);
}


///////////////////////////////////////////////////////////////////////////////
//
// UnBan
//


void vote::UnBan::starting()
{
	const auto &users(get_users());
	const auto &chan(get_chan());
	const Mask mask(get_issue());

	if(mask.get_form() != mask.INVALID)
	{
		if(!chan.lists.bans.count(mask))
			throw Exception("Can't find this mask in the ban list");

		return;
	}

	const auto &user(users.get(mask));
	const auto &bans(chan.lists.bans);
	if(!bans.count(user.mask(mask.NICK)) &&
	   !bans.count(user.mask(mask.HOST)) &&
	   !bans.count(user.mask(mask.ACCT)))
		throw Exception("Can't find any trivial match to this user in the ban list. Try the exact ban mask.");
}


void vote::UnBan::effective()
try
{
	auto &chan(get_chan());
	const Mask mask(get_issue());

	if(mask.get_form() == mask.INVALID)
	{
		const auto &users(get_users());
		const auto &user(users.get(mask));
		set_effect(chan.unban(user));
		return;
	}

	const Delta delta("-b",mask);
	chan(delta);
	set_effect(delta);
}
catch(const Exception &e)
{
	auto &chan(get_chan());
	chan << "Error attempting to unban: " << e << chan.flush;
}



///////////////////////////////////////////////////////////////////////////////
//
// Op
//


void vote::Op::starting()
{
	auto &chan(get_chan());
	if(chan.users.mode(user).has('o'))
		throw Exception("User at issue is already op");
}


void vote::Op::effective()
{
	auto &chan(get_chan());
	set_effect(chan.op(user));
}



///////////////////////////////////////////////////////////////////////////////
//
// DeOp
//


void vote::DeOp::starting()
{
	auto &chan(get_chan());
	if(!chan.users.mode(user).has('o'))
		throw Exception("User at issue is not an op");
}


void vote::DeOp::effective()
{
	auto &chan(get_chan());
	set_effect(chan.deop(user));
}



///////////////////////////////////////////////////////////////////////////////
//
// Exempt
//


void vote::Exempt::starting()
{
	const auto &chan(get_chan());
	const auto &excepts(chan.lists.excepts);
	if(excepts.count(user.mask(Mask::HOST)) && excepts.count(user.mask(Mask::ACCT)))
		throw Exception("User already has entries in the except list");
}


void vote::Exempt::effective()
{
	auto &chan(get_chan());
	set_effect(chan.except(user));
}



///////////////////////////////////////////////////////////////////////////////
//
// UnExempt
//


void vote::UnExempt::starting()
{
	const auto &users(get_users());
	const auto &chan(get_chan());
	const Mask mask(get_issue());

	if(mask.get_form() != mask.INVALID)
	{
		if(!chan.lists.excepts.count(mask))
			throw Exception("Can't find this mask in the except list");

		return;
	}

	const auto &user(users.get(mask));
	const auto &excepts(chan.lists.excepts);
	if(!excepts.count(user.mask(mask.NICK)) &&
	   !excepts.count(user.mask(mask.HOST)) &&
	   !excepts.count(user.mask(mask.ACCT)))
		throw Exception("Can't find any trivial match to this user in the except list. Try the exact mask.");
}


void vote::UnExempt::effective()
try
{
	auto &chan(get_chan());
	const Mask mask(get_issue());

	if(mask.get_form() == mask.INVALID)
	{
		const auto &users(get_users());
		const auto &user(users.get(mask));
		set_effect(chan.unexcept(user));
		return;
	}

	const Delta delta("-e",mask);
	chan(delta);
	set_effect(delta);
}
catch(const Exception &e)
{
	auto &chan(get_chan());
	chan << "Error attempting to unexempt: " << e << chan.flush;
}



///////////////////////////////////////////////////////////////////////////////
//
// Flags
//


void vote::Flags::passed()
{
	const auto toks(tokens(get_issue()));
	const auto &users(get_users());
	const auto &user(users.get(toks.at(0)));
	const Deltas deltas(toks.at(1));

	std::stringstream effect;
	effect << user.get_acct() << " " << deltas;

	auto &chan(get_chan());
	chan.flags(user,deltas);
	set_effect(effect.str());
}


void vote::Flags::expired()
{
//	const Sess &sess = get_sess();
//	const Server &serv = sess.get_server();

	const auto toks(tokens(get_issue()));

	const auto &acct(toks.at(0));
	const User user(acct,"",acct);

	const Deltas deltas(toks.at(1));
	auto &chan(get_chan());
	chan.flags(user,~deltas);
}


void vote::Flags::starting()
{
//	const Sess &sess = get_sess();
//	const Server &serv = sess.get_server();

	const auto &users(get_users());
	const auto &chan(get_chan());
	const auto &cfg(get_cfg());

	const auto toks(tokens(get_issue()));
	const auto &user(users.get(toks.at(0)));
	const Deltas deltas(toks.at(1));

	const irc::bot::Mode allowed(cfg["flags.access"]);
	for(const auto &delta : deltas)
	{
		if(char(delta) == 'f' || char(delta) == 'F')
			throw Exception("Voting on +f/+F is hardcoded to be illegal");

		if(!allowed.has(delta))
			throw Exception("Voting on this flag is not permitted");

		if(bool(delta) && chan.lists.has_flag(user,char(delta)))
			throw Exception("User at issue already has one or more of the specified flags");

		if(!bool(delta) && !chan.lists.has_flag(user,char(delta)))
			throw Exception("User at issue does not have one or more of the specified flags");
	}
}



///////////////////////////////////////////////////////////////////////////////
//
// Civis
//


void vote::Civis::starting()
try
{
	const auto &chan(get_chan());
	const auto &cfg(get_cfg());

	const Adoc excludes(cfg.get_child("civis.eligible.exclude",Adoc()));
	const auto exclude(excludes.into<std::set<std::string>>());
	if(exclude.count(user.get_acct()))
		throw Exception("User at issue is excluded by the channel configuration");

	if(chan.lists.has_flag(user,'V'))
		throw Exception("User at issue is already enfranchised");

	const auto &acct(user.get_acct());
	const auto endtime(time(NULL) - secs_cast(cfg["eligible.age"]));
	const irc::log::FilterAny filt_age([&acct,&endtime]
	(const irc::log::ClosureArgs &a)
	{
		if(a.time > endtime)
			return false;

		if(strncmp(a.type,"PRI",3) != 0)      // PRIVMSG
			return false;

		return strncmp(a.acct,acct.c_str(),16) == 0;
	});

	const irc::log::FilterAny filt_lines([&acct]
	(const irc::log::ClosureArgs &a)
	{
		if(strncmp(a.type,"PRI",3) != 0)      // PRIVMSG
			return false;

		return strncmp(a.acct,acct.c_str(),16) == 0;
	});

	if(!irc::log::atleast(get_chan_name(),filt_age,1))
		throw Exception("User at issue has not been present long enough for consideration.");

	const auto min_lines(cfg.get("eligible.lines",0U));
	const auto has_lines(irc::log::count(get_chan_name(),filt_lines));
	if(has_lines < min_lines)
		throw Exception("User at issue has ") << has_lines << " of " << min_lines << " required lines";
}
catch(const Exception &e)
{
	Chan &chan(get_chan());
	chan << "Can't start civis vote: " << e << chan.flush;
	throw;
}


void vote::Civis::passed()
{
	const Delta delta("+V");

	std::stringstream effect;
	effect << user.get_acct() << " " << delta;

	auto &chan(get_chan());
	chan.flags(user,delta);
	set_effect(effect.str());
}


void vote::Civis::expired()
{
	const Delta delta("-V");
	auto &chan(get_chan());
	chan.flags(user,delta);
}



///////////////////////////////////////////////////////////////////////////////
//
// Censure
//


void vote::Censure::starting()
{
	const Chan &chan(get_chan());
	if(!chan.lists.has_flag(user,'V'))
		throw Exception("User at issue is not a civis and cannot be censured");
}


void vote::Censure::passed()
{
	const Delta delta("-V");

	std::stringstream effect;
	effect << user.get_acct() << " " << delta;

	Chan &chan(get_chan());
	chan.flags(user,delta);
	set_effect(effect.str());
}


void vote::Censure::expired()
{
	const Delta delta("+V");
	Chan &chan(get_chan());
	chan.flags(user,delta);
}



///////////////////////////////////////////////////////////////////////////////
//
// Staff
//


void vote::Staff::starting()
{
	const auto &cfg(get_cfg());
	const auto &chan(get_chan());
	const auto &flags(cfg["staff.access"]);

	if(flags.empty())
		throw Exception("Access flags for staff are empty and must be configured");

	if(!chan.lists.has_flag(user))
		return;

	const auto &existing(chan.lists.get_flag(user));
	const bool has_all(std::all_of(flags.begin(),flags.end(),[&existing]
	(const char &flag)
	{
		return existing.has(flag);
	}));

	if(has_all)
		throw Exception("User at issue already has all of the staff flags");
}


void vote::Staff::passed()
{
	auto &chan(get_chan());
	const auto &cfg(get_cfg());
	const Deltas deltas(std::string("+") + cfg["staff.access"]);
	chan.flags(user,deltas);
	set_effect(deltas);
}


void vote::Staff::expired()
{
	auto &chan(get_chan());
	const Deltas deltas(get_effect());
	chan.flags(user,~deltas);
}



///////////////////////////////////////////////////////////////////////////////
//
// DeStaff
//


void vote::DeStaff::starting()
{
	const auto &cfg(get_cfg());
	const auto &chan(get_chan());
	const auto &flags(cfg["staff.access"]);

	if(flags.empty())
		throw Exception("Access flags for staff are empty and must be configured");

	if(!chan.lists.has_flag(user))
		throw Exception("User at issue doesn't even have any flags to remove!");
}


void vote::DeStaff::passed()
{
	auto &chan(get_chan());
	const auto &cfg(get_cfg());
	const Delta deltas(std::string("-") + cfg["staff.access"]);
	chan.flags(user,deltas);
	set_effect(deltas);
}



///////////////////////////////////////////////////////////////////////////////
//
// Opine
//


void vote::Opine::passed()
{
	using namespace colors;

	auto &chan(get_chan());
	chan << "The People of " << chan.get_name() << " decided "
	     << BOLD << get_issue() << OFF
	     << chan.flush;
}



///////////////////////////////////////////////////////////////////////////////
//
// Quote
//


void vote::Quote::starting()
{
	auto &chan(get_chan());
	if(!chan.users.has(user))
		throw Exception("You can only quote people currently online in this channel. Remember to surround nickname with < and >.");
}


void vote::Quote::passed()
{
	using namespace colors;

	auto &chan(get_chan());
	chan << "The People of " << chan.get_name() << " quoted "
	     << BOLD << get_issue() << OFF
	     << chan.flush;
}



///////////////////////////////////////////////////////////////////////////////
//
// Kick
//


void vote::Kick::passed()
{
	const auto &cfg(get_cfg());
	auto &chan(get_chan());

	{
		const auto &users(get_users());
		const auto &user(users.get(this->user.get_nick()));
		if(cfg["shield.when_away"] == "1" && user.is_away())
			throw Exception("The user is currently away and shield.when_away == 1");
	}

	chan.kick(user,"Voted off the island");
}



///////////////////////////////////////////////////////////////////////////////
//
// Invite
//


void vote::Invite::passed()
{
	auto &chan(get_chan());
	chan << "An invite to " << get_issue() << " is being sent..." << chan.flush;
	chan.invite(get_issue());
}



///////////////////////////////////////////////////////////////////////////////
//
// Mode
//


void vote::Mode::passed()
{
	const auto &sess(get_sess());
	const auto &serv(sess.get_server());
	const Deltas deltas(get_issue(),serv);

	auto &chan(get_chan());
	chan(deltas);
	set_effect(deltas);
}


void vote::Mode::starting()
{
	const auto &sess(get_sess());
	const auto &users(get_users());
	const auto &serv(sess.get_server());
	const auto &myself(users.get(sess.get_nick()));

	const Deltas deltas(get_issue(),serv);
	deltas.validate_chan(serv);

	for(const auto &delta : deltas)
		if(myself.is_myself(std::get<Delta::MASK>(delta)))
			throw Exception("One of the mode deltas matches myself.");
}



///////////////////////////////////////////////////////////////////////////////
//
// Appeal
//


void vote::Appeal::passed()
{
	const auto &sess(get_sess());
	const auto &serv(sess.get_server());
	const Deltas deltas(get_issue(),serv);

	auto &chan(get_chan());
	chan(deltas);
	set_effect(deltas);
}


void vote::Appeal::starting()
{
	const auto &sess(get_sess());
	const auto &serv(sess.get_server());
	const Deltas deltas(get_issue(),serv);
	deltas.validate_chan(serv);

	const auto &user(get_user());
	cast(Ballot::NAY,user);
}



///////////////////////////////////////////////////////////////////////////////
//
// Trial
//


void vote::Trial::starting()
{
	const auto &sess(get_sess());
	const auto &serv(sess.get_server());
	const Deltas deltas(get_issue(),serv);
	set_effect(deltas);

	const auto &cfg(get_cfg());
	if(cfg.get("visible.flair",false))
	{
		auto &chan(get_chan());
		chan << "An arrest has been made by " << get_user_nick() << ". ";
		chan << "The accused now stands trial by a jury of their peers in due process.";
		chan << chan.flush;
	}
}



///////////////////////////////////////////////////////////////////////////////
//
// Topic
//


void vote::Topic::passed()
{
	auto &chan(get_chan());
	chan.topic(get_issue());
}


void vote::Topic::starting()
{
	const auto &sess(get_sess());
	if(get_issue().size() > sess.get_isupport().get("TOPICLEN",256U))
		throw Exception("Topic length exceeds maximum for this server.");
}



///////////////////////////////////////////////////////////////////////////////
//
// Config
//


void vote::Config::starting()
{
	using namespace colors;

	Chan &chan(get_chan());
	const Adoc &cfg(chan.get());

	if(val.empty())
	{
		chan << "Note: vote deletes variable [" << BOLD << key << OFF << "] "
		     << BOLD << "and all child variables" << OFF << "."
		     << chan.flush;
	}

	if(!val.empty() && oper == "=")
	{
		if(cfg[key] == val)
			throw Exception("Variable already set to that value.");
	}

	chan << "Note: "
	     << UNDER2 << "Changing the configuration can be " << BOLD << "DANGEROUS" << OFF << ", "
	     << "potentially breaking the ability for future votes to revert this change."
	     << chan.flush;
}


void vote::Config::passed()
{
	Chan &chan(get_chan());
	Adoc cfg(chan.get());

	const auto toks(tokens(val));
	if(oper == "=" && toks.empty())
	{
		if(!cfg.remove(key))
			throw Exception("The configuration key was not found.");
	}
	else if(oper == "=" && toks.size() == 1)
	{
		cfg.put(key,val);
	}
	else if(oper == "=" && toks.size() > 1)
	{
		Adoc val;
		for(const auto &tok : toks)
			val.push(tok);

		cfg.put_child(key,val);
	}
	else if(oper == "+=" && !toks.empty())
	{
		Adoc val(cfg.get_child(key,Adoc()));
		for(const auto &tok : toks)
			val.push(tok);

		cfg.put_child(key,val);
	}
	else if(oper == "-=" && !toks.empty())
	{
		Adoc dst;
		const Adoc src(cfg.get_child(key,Adoc()));
		for(const auto &pair : src)
		{
			const auto &val(pair.second.get("",std::string{}));
			if(std::find(toks.begin(),toks.end(),val) == toks.end())
				dst.push(val);
		}

		cfg.put_child(key,dst);
	}
	else throw Exception("Configuration update could not be parsed");

	chan.set(cfg);
}



///////////////////////////////////////////////////////////////////////////////
//
// Attribute: NickIssue - All votes with a nickname as the issue
//


void NickIssue::starting()
{
	if(user.is_myself())
		throw Exception("http://en.wikipedia.org/wiki/Leviathan_(book)");

	auto &chan(get_chan());
	if(!chan.users.has(user))
		throw Exception("You cannot make a vote about someone in another channel.");

	const auto toks(tokens(get_issue()));
	if(toks.size() > 1)
		throw Exception("This vote requires only a nickname as the issue.");

	user.whois();
}


void NickIssue::event_nick(User &user,
                           const std::string &old)
{
	if(this->user.get_nick() == old)
		this->user = user;
}



///////////////////////////////////////////////////////////////////////////////
//
// Attribute: AcctIssue - All votes with an account name as the issue,
// exception when starting (constructor) where a nickname must be resolved
// into an account name.
//


void AcctIssue::event_nick(User &user,
                           const std::string &old)
{
	if(this->user.get_nick() == old && this->user.get_acct() == user.get_acct())
		this->user = user;

	if(this->user.get_nick().empty() && this->user.get_acct() == user.get_acct())
		this->user = user;
}



///////////////////////////////////////////////////////////////////////////////
//
// Attribute: ModeEffect - All effect strings are invertible mode Deltas
//


void ModeEffect::expired()
{
	const auto &sess(get_sess());
	const auto &serv(sess.get_server());
	if(get_effect().empty())
		return;

	const Deltas deltas(get_effect(),serv);
	auto &chan(get_chan());
	chan(~deltas);
}
