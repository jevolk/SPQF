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



///////////////////////////////////////////////////////////////////////////////
//
// UnQuiet
//

void vote::UnQuiet::starting()
{
	const std::string &victim = get_issue();
	User &user = get_users().get(victim);
	user.whois();
}


void vote::UnQuiet::passed()
{
	Chan &chan = get_chan();
	const Adoc &cfg = get_cfg();
	const User &user = get_users().get(get_issue());
	chan.unquiet(user);
}



///////////////////////////////////////////////////////////////////////////////
//
// Quiet
//

void vote::Quiet::starting()
{
	const std::string &victim = get_issue();
	User &user = get_users().get(victim);

	if(user.is_myself())
		throw Exception("http://en.wikipedia.org/wiki/Areopagitica");

	user.whois();
}


void vote::Quiet::passed()
{
	Chan &chan = get_chan();
	const Adoc &cfg = get_cfg();
	const User &user = get_users().get(get_issue());
	chan.quiet(user);
}



///////////////////////////////////////////////////////////////////////////////
//
// Ban
//

void vote::Ban::starting()
{
	const std::string &victim = get_issue();
	User &user = get_users().get(victim);

	if(user.is_myself())
		throw Exception("http://en.wikipedia.org/wiki/Leviathan_(book)");

	user.whois();
}


void vote::Ban::passed()
{
	Chan &chan = get_chan();
	const Adoc &cfg = get_cfg();
	const User &user = get_users().get(get_issue());
	chan.ban(user);
	chan.remove(user,"And I ain't even mad");
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

void vote::Kick::starting()
{
	const std::string &victim = get_issue();
	User &user = get_users().get(victim);

	if(user.is_myself())
		throw Exception("GNU philosophy 101: You're not free to be unfree.");

	user.whois();
}


void vote::Kick::passed()
{
	Chan &chan = get_chan();
	const Adoc &cfg = get_cfg();
	const User &user = get_users().get(get_issue());

	if(cfg["shield.when_away"] == "1" && user.is_away())
		throw Exception("The user is currently away and shield.when_away == 1");

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
	Chan &chan = get_chan();
	chan.mode(get_issue());
}


void vote::Mode::starting()
{
	const Sess &sess = get_sess();
	const Users &users = get_users();
	const Server &serv = sess.get_server();
	const User &myself = users.get(sess.get_nick());

	const Deltas deltas(get_issue(),serv);
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

	if(val.empty())
		chan << "Note: vote deletes variable [" << BOLD << key << OFF << "] "
		     << BOLD << "and all child variables" << OFF << "."
		     << flush;

	// Hacking manual type checking here  //TODO: key typing
	if(!val.empty())
	{
		// Key conforms to types that want mode strings
		if(key.find(".access") != std::string::npos ||
		   key.find(".mode") != std::string::npos)
		{

			if(!std::all_of(val.begin(),val.end(),[&](auto&& c){ return std::isalpha(c,locale); }))
				throw Exception("Must use letters only for value for this key.");

		} else {

			if(!std::all_of(val.begin(),val.end(),[&](auto&& c){ return std::isdigit(c,locale); }))
				throw Exception("Must use a numerical value for this key.");

		}
	}

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
