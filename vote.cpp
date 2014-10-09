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
	chan << BOLD << "Voting has started!" << OFF << " Issue " << BOLD << "#" << get_id() << OFF << ": "
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

	if(yea.size() < cfg.get<uint>("min_yea"))
	{
		if(cfg.get<bool>("result.ack_chan"))
			chan << (*this) << ": "
			     << "Failed to reach minimum number of yes votes: "
			     << FG::GREEN << yea.size() << OFF
			     << " of "
			     << FG::GREEN << BOLD << cfg.get<uint>("min_yea") << OFF
			     << " required."
			     << flush;

		failed();
		return;
	}

	if(yea.size() < required())
	{
		if(cfg.get<bool>("result.ack_chan"))
			chan << (*this) << ": "
			     << FG::WHITE << BG::RED << BOLD << "The nays have it." << OFF << "."
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
	const auto min_votes = cfg.get<uint>("min_votes");
	const float turnout = cfg.get<float>("turnout");
	if(turnout <= 0.0)
		return min_votes;

	const Chan &chan = get_chan();
	const float eligible = chan.count_logged_in();
	const uint req = ceil(eligible * turnout);
	return std::max(min_votes,req);
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
	put("ballot.ack_chan",0);
	put("ballot.ack_priv",1);
	put("ballot.rej_chan",0);
	put("ballot.rej_priv",1);
	put("ballot.last_spoke",900);
	put("ballot.first_spoke",1800);
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