/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include <forward_list>

// libircbot irc::bot::
#include "ircbot/bot.h"

// SPQF
using namespace irc::bot;
#include "vote.h"
#include "votes.h"
#include "voting.h"
#include "respub.h"


Vote::Vote(const id_t &id,
           const Sess &sess,
           Chans &chans,
           Users &users,
           Logs &logs,
           Chan &chan,
           User &user,
           const std::string &issue,
           Adoc cfg):
sess(sess),
chans(chans),
users(users),
logs(logs),
id(id),
cfg([&]() -> Adoc
{
	if(cfg.empty())
		return DefaultConfig::configure(chan);

	DefaultConfig::configure(cfg);
	return cfg;
}()),
began(time(NULL)),
chan(chan.get_name()),
user(user.get_acct()),
issue(issue)
{


}


void Vote::cancel()
{
	canceled();

	if(cfg.get<bool>("result.ack_chan"))
	{
		Chan &chan = get_chan();
		chan << "The vote " << (*this) << " has been canceled." << flush;
	}
}


void Vote::start()
{
	using namespace colors;

	starting();

	auto &chan = get_chan();
	chan << BOLD << "Voting has started!" << OFF << " Issue " << (*this) << ": "
	     << "You have " << BOLD << cfg.get<uint>("duration") << OFF << " seconds to vote! "
	     << "Type or PM: "
	     << BOLD << FG::GREEN << "!vote y" << OFF << " " << BOLD << get_id() << OFF
	     << " or "
	     << BOLD << FG::RED << "!vote n" << OFF << " " << BOLD << get_id() << OFF
	     << flush;
}


void Vote::finish()
try
{
	using namespace colors;

	auto &chan = get_chan();

	if(total() < minimum())
	{
		if(cfg.get<bool>("result.ack_chan"))
			chan << (*this) << ": "
			     << "Failed to reach minimum number of votes: "
			     << BOLD << total() << OFF
			     << " of "
			     << BOLD << minimum() << OFF
			     << " required."
			     << flush;

		failed();
		return;
	}

	if(yea.size() < required())
	{
		if(cfg.get<bool>("result.ack_chan"))
			chan << (*this) << ": "
			     << FG::WHITE << BG::RED << BOLD << "The nays have it." << OFF
			     << " Yeas: " << FG::GREEN << yea.size() << OFF << "."
			     << " Nays: " << FG::RED << BOLD << nay.size() << OFF << "."
			     << " Required at least: " << BOLD << required() << OFF << " yeas."
			     << flush;

		failed();
		return;
	}

	if(cfg.get<bool>("result.ack_chan"))
		chan << (*this) << ": "
		     << FG::WHITE << BG::GREEN << BOLD << "The yeas have it." << OFF
		     << " Yeas: " << FG::GREEN << BOLD << yea.size() << OFF << "."
		     << " Nays: " << FG::RED << nay.size() << OFF << "."
		     << flush;

	passed();
}
catch(const Exception &e)
{
	if(cfg.get<bool>("result.ack_chan"))
	{
		auto &chan = get_chan();
		chan << "The vote " << (*this) << " was rejected: " << e << flush;
	}
}


void Vote::vote(const Ballot &ballot,
                User &user)
try
{
	using namespace colors;

	Chan &chan = get_chan();
	switch(cast(ballot,user))
	{
		case ADDED:
			if(cfg.get<bool>("ballot.ack_chan"))
				chan << user << "Thanks for casting your vote on " << (*this) << "!" << flush;

			if(cfg.get<bool>("ballot.ack_priv"))
				user << "Thanks for casting your vote on " << (*this) << "!" << flush;

			break;

		case CHANGED:
			if(cfg.get<bool>("ballot.ack_chan"))
				chan << user << "You have changed your vote on " << (*this) << "!" << flush;

			if(cfg.get<bool>("ballot.ack_priv"))
				user << "You have changed your vote on " << (*this) << "!" << flush;

			break;
	}
}
catch(const Exception &e)
{
	using namespace colors;

	if(cfg.get<bool>("ballot.rej_chan"))
	{
		Chan &chan = get_chan();
		chan << user << "Your vote was not accepted for " << (*this) << ": " << e << flush;
	}

	if(cfg.get<bool>("ballot.rej_priv"))
		user << "Your vote was not accepted for " << (*this) << ": " << e << flush;
}


Vote::Stat Vote::cast(const Ballot &ballot,
                      User &user)
{
	enfranchise(ballot,user);
	proffer(ballot,user);

	const std::string &acct = user.get_acct();
	switch(ballot)
	{
		case YEA:
			return !yea.emplace(acct).second?  throw Exception("You have already voted yea."):
			       nay.erase(acct)?            CHANGED:
			                                   ADDED;
		case NAY:
			return !nay.emplace(acct).second?  throw Exception("You have already voted nay."):
			       yea.erase(acct)?            CHANGED:
			                                   ADDED;
		default:
			throw Exception("Ballot type not accepted.");
	}
}


void Vote::enfranchise(const Ballot &ballot,
                       User &user)
{
	Logs &logs = get_logs();
	Chan &chan = get_chan();
	const Adoc &cfg = get_cfg();
	const time_t eftime = cfg.get<time_t>("enfranchise.time");
	const size_t eflines = cfg.get<size_t>("enfranchise.lines");

	Logs::SimpleFilter filter;
	filter.acct = user.get_acct();
	filter.time.first = get_began() - eftime;
	filter.time.second = get_began();
	filter.type = "CHA";

	if(!logs.atleast(chan,filter,eflines))
		throw Exception("You have not been active enough to participate in this vote.");
}


bool Vote::voted(const User &user)
const
{
	const std::string &acct = user.get_acct();
	return yea.count(acct) || nay.count(acct);
}


Vote::Ballot Vote::position(const User &user)
const
{
	const std::string &acct = user.get_acct();
	return yea.count(acct)? YEA:
	       nay.count(acct)? NAY:
	                        throw Exception("No position taken.");
}


uint Vote::required()
const
{
	const auto plura = plurality();
	const auto min_yea = cfg.get<uint>("min_yea");
	return std::max(min_yea,plura);
}


uint Vote::minimum()
const
{
	std::vector<uint> sel
	{{
		cfg.get<uint>("min_yea"),
		cfg.get<uint>("min_votes"),
	}};

	if(cfg.get<float>("turnout") <= 0.0)
		return *std::max_element(sel.begin(),sel.end());

	const Chan &chan = get_chan();
	const float eligible = chan.count_logged_in();
	sel.emplace_back(ceil(eligible * cfg.get<float>("turnout")));
	return *std::max_element(sel.begin(),sel.end());
}


uint Vote::plurality()
const
{
	const float total = this->total();
	const auto plura = cfg.get<float>("plurality");
	return ceil(total * plura);
}


bool Vote::disabled()
const
{
	if(!cfg.has_child(type()))
		return false;

	return cfg.get_child(type()).get("disable","0") == "1";
}


Locutor &operator<<(Locutor &locutor,
                    const Vote &vote)
{
	using namespace colors;

	locutor << "#" << BOLD << vote.get_id() << OFF;
	return locutor;
}



DefaultConfig::DefaultConfig()
{
	// config.vote
	put("max_active",16);
	put("max_per_user",1);
	put("min_votes",1);
	put("min_yea",1);
	put("turnout",0.00);
	put("duration",30);
	put("plurality",0.51);
	put("enfranchise.time",900);
	put("enfranchise.lines",3);
	put("ballot.ack_chan",0);
	put("ballot.ack_priv",1);
	put("ballot.rej_chan",0);
	put("ballot.rej_priv",1);
	put("result.ack_chan",1);
}


Adoc DefaultConfig::configure(Chan &chan)
{
	Adoc cfg = chan.get("config.vote");

	if(configure(cfg))
	{
		Adoc chan_cfg;
		chan_cfg.put_child("config.vote",cfg);
		chan.set(chan_cfg);
	}

	return cfg;
}


uint DefaultConfig::configure(Adoc &cfg)
{
	static const DefaultConfig default_config;

	const std::function<size_t (const Adoc &def, Adoc &cfg)> recurse = [&]
	(const Adoc &def, Adoc &cfg) -> uint
	{
		uint ret = 0;
		for(const auto &pair : def)
		{
			const std::string &key = pair.first;
			const auto &subtree = pair.second;

			if(!subtree.empty())
			{
				Adoc sub = cfg.get_child(key,Adoc());
				ret += recurse(subtree,sub);
				cfg.put_child(key,sub);
			}
			else if(!cfg.has_child(key))
			{
				cfg.put_child(key,subtree);
				ret++;
			}
		}

		return ret;
	};

	return recurse(default_config,cfg);
}
