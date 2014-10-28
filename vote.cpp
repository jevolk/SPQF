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


Vote::Vote(const std::string &type,
           const id_t &id,
           Adb &adb,
           Sess &sess,
           Chans &chans,
           Users &users,
           Logs &logs,
           Chan &chan,
           User &user,
           const std::string &issue,
           const Adoc &cfg):
Acct(adb,&this->id),
sess(&sess),
chans(&chans),
users(&users),
logs(&logs),
id(lex_cast(id)),
type(type),
chan(chan.get_name()),
nick(user.get_nick()),
acct(user.get_acct()),
issue(strip_args(issue,ARG_KEYED)),
cfg([&]
{
	// Default config
	Adoc ret;
	ret.put("disable",0);
	ret.put("for",3600);
	ret.put("max_active",16);
	ret.put("max_per_user",1);
	ret.put("min_ballots",1);
	ret.put("min_yea",1);
	ret.put("turnout",0.00);
	ret.put("duration",30);
	ret.put("plurality",0.51);
	ret.put("speaker.access","");
	ret.put("speaker.mode","");
	ret.put("veto.access","");
	ret.put("veto.mode","v");
	ret.put("veto.min",1);
	ret.put("veto.quick",1);
	ret.put("enfranchise.age",1800);
	ret.put("enfranchise.lines",0);
	ret.put("enfranchise.access","");
	ret.put("qualify.age",900);
	ret.put("qualify.lines",0);
	ret.put("qualify.access","");
	ret.put("ballot.ack_chan",0);
	ret.put("ballot.ack_priv",1);
	ret.put("ballot.rej_chan",0);
	ret.put("ballot.rej_priv",1);
	ret.put("result.ack_chan",1);
	ret.put("visible.ballots",0);
	ret.put("visible.veto",1);
	ret.merge(chan.get("config.vote"));                      // Overwrite defaults with saved config
	chan.set("config.vote",ret);                             // Write back combined result to db
	ret.merge(ret.get_child(type,Adoc()));                   // Import type-specifc overrides up to main
	ret.merge({Adoc::arg_ctor,issue,ARG_KEYED,ARG_VALUED});  // Any vote-time options from user.
	ret.merge(cfg);                                          // Any overrides trumping all.
	return ret;
}()),
began(0),
ended(0)
{
	if(disabled())
		throw Exception("Votes of this type are disabled by the configuration.");

	// Initiation implies yes vote
	yea.emplace(get_user_acct());
	hosts.emplace(user.get_host());
}


Vote::Vote(const std::string &type,
           const id_t &id,
           Adb &adb,
           Sess *const &sess,
           Chans *const &chans,
           Users *const &users,
           Logs *const &logs)
try:
Acct(adb,&this->id),
sess(sess),
chans(chans),
users(users),
logs(logs),
id(lex_cast(id)),
type(get_val("type")),
chan(get_val("chan")),
nick(get_val("nick")),
acct(get_val("acct")),
issue(get_val("issue")),
cfg(get("cfg")),
began(get_val<time_t>("began")),
ended(get_val<time_t>("ended")),
reason(get_val("reason")),
effect(get_val("effect")),
yea(get("yea").into(yea)),
nay(get("nay").into(nay)),
veto(get("veto").into(veto)),
hosts(get("hosts").into(hosts))
{
	if(cfg.empty())
		throw Assertive("The configuration for this vote is missing and required.");
}
catch(const std::runtime_error &e)
{
	throw Assertive("Failed to reconstruct Vote data: ") << e.what();
}


Vote::operator Adoc()
const
{
	Adoc yea, nay, veto, hosts;
	yea.push(this->yea.begin(),this->yea.end());
	nay.push(this->nay.begin(),this->nay.end());
	veto.push(this->veto.begin(),this->veto.end());
	hosts.push(this->hosts.begin(),this->hosts.end());

	Adoc doc;
	doc.put("id",get_id());
	doc.put("type",get_type());
	doc.put("chan",get_chan_name());
	doc.put("nick",get_user_nick());
	doc.put("acct",get_user_acct());
	doc.put("issue",get_issue());
	doc.put("began",get_began());
	doc.put("ended",get_ended());
	doc.put("reason",get_reason());
	doc.put("effect",get_effect());
	doc.put_child("cfg",get_cfg());
	doc.put_child("yea",yea);
	doc.put_child("nay",nay);
	doc.put_child("veto",veto);
	doc.put_child("hosts",hosts);

	return doc;
}


void Vote::cancel()
{
	set_ended();
	set_reason("canceled");
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
	set_began();

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

	set_ended();
	const scope s([&]{ save(); });

	if(interceded())
	{
		auto &chan = get_chan();
		if(cfg.get<bool>("result.ack_chan"))
			chan << "The vote " << (*this) << " has been vetoed." << flush;

		set_reason("vetoed");
		vetoed();
		return;
	}

	if(total() < minimum())
	{
		auto &chan = get_chan();
		if(cfg.get<bool>("result.ack_chan"))
			chan << (*this) << ": "
			     << "Failed to reach minimum number of votes: "
			     << BOLD << total() << OFF
			     << " of "
			     << BOLD << minimum() << OFF
			     << " required."
			     << flush;

		set_reason("minimums");
		failed();
		return;
	}

	if(yea.size() < required())
	{
		auto &chan = get_chan();
		if(cfg.get<bool>("result.ack_chan"))
			chan << (*this) << ": "
			     << FG::WHITE << BG::RED << BOLD << "The nays have it." << OFF
			     << " Yeas: " << FG::GREEN << yea.size() << OFF << "."
			     << " Nays: " << FG::RED << BOLD << nay.size() << OFF << "."
			     << " Required at least: " << BOLD << required() << OFF << " yeas."
			     << flush;

		set_reason("plurality");
		failed();
		return;
	}

	auto &chan = get_chan();
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
	set_reason(e.what());

	if(cfg.get<bool>("result.ack_chan"))
	{
		auto &chan = get_chan();
		chan << "The vote " << (*this) << " was rejected: " << e << flush;
	}
}


void Vote::event_vote(User &user,
                      const Ballot &ballot)
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

	save();
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

	Logs &logs = get_logs();
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

	Logs &logs = get_logs();
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
	return chan.users.mode(user).any(mode);
}


bool Vote::has_access(const User &user,
                      const Mode &flags)
const
{
	const Chan &chan = get_chan();
	if(!chan.lists.has_flag(user))
		return false;

	const auto &f = chan.lists.get_flag(user);
	return f.get_flags().any(flags);
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
	const float eligible = chan.users.count_logged_in();
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


Locutor &operator<<(Locutor &locutor,
                    const Vote &vote)
{
	using namespace colors;

	return (locutor << "#" << BOLD << vote.get_id() << OFF);
}
