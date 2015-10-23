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



///////////////////////////////////////////////////////////////////////////////
//
// Import - Copy a configuration from another channel
//


void vote::Import::starting()
{
	static const auto usage = "!vote import <#chan> <nickname of bot>";

	if(get_target_bot().empty())
		throw Exception("You must hint the nickname of the bot: ") << usage;

	if(get_target_chan().empty())
		throw Exception("You must specify a channel to fetch from: ") << usage;

	if(tolower(get_target_chan()) == tolower(get_chan_name()))
		throw Exception("The source of the config must be a different channel.");

	const auto &sess = get_sess();
	const auto &isup = sess.get_isupport();
	const auto type = get_target_chan().substr(0,1);
	if(!isup.find_in("CHANTYPES",type))
		throw Exception("You must specify a valid channel type for this server.");

	if(tolower(get_target_bot()) == tolower(sess.get_nick()))
	{
		Chans &chans = get_chans();
		Chan &chan = chans.get(get_target_chan());
		received << chan.get("config.vote");
		return;
	}

	Locutor bot(get_sess(),get_target_bot());
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

	Chan &chan = get_chan();
	chan.set("config.vote",received.str());

	chan << "Applied new configuration"
	     << " (received " << received.str().size() << " bytes)"
	     << chan.flush;
}



///////////////////////////////////////////////////////////////////////////////
//
// UnQuiet
//


void vote::UnQuiet::passed()
{
	Chan &chan = get_chan();
	chan.unquiet(user);
}



///////////////////////////////////////////////////////////////////////////////
//
// Quiet
//


void vote::Quiet::passed()
{
	Chan &chan = get_chan();
	set_effect(chan.quiet(user));
}



///////////////////////////////////////////////////////////////////////////////
//
// DeVoice
//


void vote::DeVoice::passed()
{
	Chan &chan = get_chan();
	chan.devoice(user);
}



///////////////////////////////////////////////////////////////////////////////
//
// Voice
//


void vote::Voice::passed()
{
	Chan &chan = get_chan();
	set_effect(chan.voice(user));
}



///////////////////////////////////////////////////////////////////////////////
//
// Ban
//


void vote::Ban::passed()
{
	Chan &chan = get_chan();
	set_effect(chan.ban(user));
	chan.remove(user,"And I ain't even mad");
}



///////////////////////////////////////////////////////////////////////////////
//
// Flags
//


void vote::Flags::passed()
{
	const auto toks(tokens(get_issue()));
	const Users &users(get_users());
	const User &user(users.get(toks.at(0)));
	const Deltas deltas(toks.at(1));

	std::stringstream effect;
	effect << user.get_acct() << " " << deltas;

	Chan &chan = get_chan();
	chan.flags(user,deltas);
	set_effect(effect.str());
}


void vote::Flags::expired()
{
//	const Sess &sess = get_sess();
//	const Server &serv = sess.get_server();

	const auto toks(tokens(get_issue()));

	const auto &acct(toks.at(0));
	const User user(&get_adb(),&get_sess(),nullptr,acct,"",acct);

	Deltas deltas(toks.at(1));
	deltas.inv_signs();

	Chan &chan(get_chan());
	chan.flags(user,deltas);
}


void vote::Flags::starting()
{
//	const Sess &sess = get_sess();
//	const Server &serv = sess.get_server();

	const auto toks(tokens(get_issue()));
	const Users &users(get_users());
	const User &user(users.get(toks.at(0)));
	const Deltas deltas(toks.at(1));

	const Adoc &cfg(get_cfg());
	const irc::bot::Mode allowed(cfg["flags.access"]);
	for(const auto &delta : deltas)
		if(!allowed.has(delta))
			throw Exception("Voting on this flag is not permitted");
}



///////////////////////////////////////////////////////////////////////////////
//
// Opine
//


void vote::Opine::passed()
{
	using namespace colors;

	Chan &chan = get_chan();
	chan << "The People of " << chan.get_name() << " decided "
	     << BOLD << get_issue() << OFF
	     << flush;
}



///////////////////////////////////////////////////////////////////////////////
//
// Kick
//


void vote::Kick::passed()
{
	const Adoc &cfg = get_cfg();
	Chan &chan = get_chan();

	{
		const Users &users = get_users();
		const User &user = users.get(this->user.get_nick());
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
	Chan &chan = get_chan();
	chan << "An invite to " << get_issue() << " is being sent..." << flush;
	chan.invite(get_issue());
}



///////////////////////////////////////////////////////////////////////////////
//
// Mode
//


void vote::Mode::passed()
{
	const Sess &sess = get_sess();
	const Server &serv = sess.get_server();

	const Deltas deltas(get_issue(),serv);
	Chan &chan(get_chan());
	chan(deltas);
	set_effect(deltas);
}


void vote::Mode::starting()
{
	const Sess &sess = get_sess();
	const Users &users = get_users();
	const Server &serv = sess.get_server();
	const User &myself = users.get(sess.get_nick());

	const Deltas deltas(get_issue(),serv);
	deltas.validate_chan(serv);

	for(const Delta delta : deltas)
		if(myself.is_myself(std::get<Delta::MASK>(delta)))
			throw Exception("One of the mode deltas matches myself.");
}



///////////////////////////////////////////////////////////////////////////////
//
// Topic
//


void vote::Topic::passed()
{
	Chan &chan = get_chan();
	chan.topic(get_issue());
}


void vote::Topic::starting()
{
	const Sess &sess = get_sess();
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

	Chan &chan = get_chan();
	const Adoc &cfg = chan.get();

	if(!val.empty() && cfg[key] == val)
		throw Exception("Variable already set to that value.");

	if(!val.empty())
	{
		Adoc tmp;
		tmp.put(key,val);
		valid(tmp);            // throws if
	}
	else chan << "Note: vote deletes variable [" << BOLD << key << OFF << "] "
	          << BOLD << "and all child variables" << OFF << "."
	          << flush;

	chan << "Note: "
	     << UNDER2 << "Changing the configuration can be " << BOLD << "DANGEROUS" << OFF << ", "
	     << "potentially breaking the ability for future votes to revert this change."
	     << flush;
}


void vote::Config::passed()
{
	Chan &chan = get_chan();
	Adoc cfg = chan.get();

	if(!val.empty())
		cfg.put(key,val);
	else if(!cfg.remove(key))
		throw Exception("The configuration key was not found.");

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

	Chan &chan = get_chan();
	if(!chan.users.has(user))
		throw Exception("You cannot make a vote about someone in another channel.");

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
// Attribute: ModeEffect - All effect strings are invertible mode Deltas
//


void ModeEffect::expired()
{
	const Sess &sess = get_sess();
	const Server &serv = sess.get_server();

	Deltas deltas(get_effect(),serv);
	deltas.inv_signs();

	Chan &chan(get_chan());
	chan(deltas);
}
