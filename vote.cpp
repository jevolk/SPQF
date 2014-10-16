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
nick(user.get_nick()),
acct(user.get_acct()),
issue(issue)
{
	if(disabled())
		throw Exception("Votes of this type are disabled by the configuration.");

	// Initiation implies yes vote
	yea.emplace(get_user_acct());
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

	if(interceded())
	{
		if(cfg.get<bool>("result.ack_chan"))
			chan << "The vote " << (*this) << " has been vetoed." << flush;

		vetoed();
		return;
	}

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
			hosts.emplace(user.get_host());

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
	if(!voted(user) && voted_host(user.get_host()) > 0)
		throw Exception("You can not cast another vote from this hostname.");

	if(!voted(user) && !enfranchised(user))
		throw Exception("You are not yet enfranchised in this channel.");

	if(!voted(user) && !qualified(user))
		throw Exception("You have not been active enough qualify for this vote.");

	if(ballot == NAY && intercession(user))
		veto.emplace(user.get_acct());

	proffer(ballot,user);

	switch(ballot)
	{
		case YEA:
			return !yea.emplace(user.get_acct()).second?  throw Exception("You have already voted yea."):
			       nay.erase(user.get_acct())?            CHANGED:
			                                              ADDED;
		case NAY:
			return !nay.emplace(user.get_acct()).second?  throw Exception("You have already voted nay."):
			       yea.erase(user.get_acct())?            CHANGED:
			                                              ADDED;
		default:
			throw Exception("Ballot type not accepted.");
	}
}


bool Vote::voted(const User &user)
const
{
	return voted_acct(user.get_acct()) ||
	       voted_host(user.get_host());
}


bool Vote::speaker(const User &user)
const
{
	const Adoc &cfg = get_cfg();
	return (!cfg.has("speaker.access") || has_access(user,cfg["speaker.access"])) &&
	       (!cfg.has("speaker.mode") || has_mode(user,cfg["speaker.mode"]));
}


bool Vote::qualified(const User &user)
const
{
	const Adoc &cfg = get_cfg();
	if(has_access(user,cfg["qualify.access"]))
		return true;

	Logs::SimpleFilter filt;
	filt.acct = user.get_acct();
	filt.time.first = get_began() - cfg.get<uint>("qualify.age");
	filt.time.second = get_began();
	filt.type = "CHA";

	const Chan &chan = get_chan();
	return logs.atleast(chan,filt,cfg.get<uint>("qualify.lines"));
}


bool Vote::enfranchised(const User &user)
const
{
	const Adoc &cfg = get_cfg();
	if(cfg.has("enfranchise.access"))
		return has_access(user,cfg["enfranchise.access"]);

	Logs::SimpleFilter filt;
	filt.acct = user.get_acct();
	filt.time.first = 0;
	filt.time.second = get_began() - cfg.get<uint>("enfranchise.age");
	filt.type = "CHA";

	const Chan &chan = get_chan();
	return logs.atleast(chan,filt,cfg.get<uint>("enfranchise.lines"));
}


bool Vote::intercession(const User &user)
const
{
	const Adoc &cfg = get_cfg();
	if(cfg.has("veto.mode") && !has_mode(user,cfg["veto.mode"]))
		return false;

	if(cfg.has("veto.access") && !has_access(user,cfg["veto.access"]))
		return false;

	return cfg.has("veto.access") || cfg.has("veto.mode");
}


bool Vote::has_mode(const User &user,
                    const Mode &mode)
const
{
	const Chan &chan = get_chan();
	return chan.get_mode(user).any(mode);
}


bool Vote::has_access(const User &user,
                      const Mode &flags)
const
{
	const Chan &chan = get_chan();
	const auto &cf = chan.get_flags();
	const auto it = cf.find({user.get_acct()});
	return it != cf.end()? it->get_flags().any(flags) : false;
}


uint Vote::voted_host(const std::string &host)
const
{
	return hosts.count(host);
}


bool Vote::voted_acct(const std::string &acct)
const
{
	return yea.count(acct) || nay.count(acct);
}


Vote::Ballot Vote::position(const std::string &acct)
const
{
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
		cfg.get<uint>("min_ballots"),
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
	const float &total = this->total();
	const auto plura = cfg.get<float>("plurality");
	return ceil(total * plura);
}


bool Vote::interceded()
const
{
	const auto vmin = std::max(cfg.get<uint>("veto.min"),1U);
	if(num_vetoes() < vmin)
		return false;

	const auto quick = cfg.get<bool>("veto.quick");
	return quick? true : !remaining();
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
	put("min_ballots",1);
	put("min_yea",1);
	put("turnout",0.00);
	put("duration",30);
	put("plurality",0.51);
	put("speaker.access","");
	put("speaker.mode","");
	put("veto.access","");
	put("veto.mode","v");
	put("veto.min",1);
	put("veto.quick",1);
	put("enfranchise.age",1800);
	put("enfranchise.lines",0);
	put("enfranchise.access","");
	put("qualify.age",900);
	put("qualify.lines",0);
	put("qualify.access","");
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
