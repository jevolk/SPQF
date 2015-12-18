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


decltype(ystr) ystr                              { "yea", "yes", "yea", "Y", "y"                   };
decltype(nstr) nstr                              { "nay", "no", "N", "n"                           };

decltype(Vote::ARG_KEYED) Vote::ARG_KEYED        { "--"                                            };
decltype(Vote::ARG_VALUED) Vote::ARG_VALUED      { "="                                             };


Vote::Vote(const std::string &type,
           const id_t &id,
           Adb &adb,
           Chan &chan,
           User &user,
           const std::string &issue,
           const Adoc &cfg):
Acct(&this->id,&adb),
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
	ret.put("audibles","");
	ret.put("limit.active",16);
	ret.put("limit.user",1);
	ret.put("limit.age",0);
	ret.put("limit.quorum.age",120);
	ret.put("limit.plurality.age",1800);
	ret.put("quorum.ballots",1);
	ret.put("quorum.yea",1);
	ret.put("quorum.age",0);
	ret.put("quorum.lines",0);
	ret.put("quorum.turnout",0.00);
	ret.put("quorum.plurality",0.51);
	ret.put("quorum.quick",0);
	ret.put("quorum.prejudice",0);
	ret.put("duration",30);
	ret.put("speaker.access","");
	ret.put("speaker.mode","");
	ret.put("veto.access","");
	ret.put("veto.mode","o");
	ret.put("veto.quorum",1);
	ret.put("veto.quick",1);
	ret.put("enfranchise.age",1800);
	ret.put("enfranchise.lines",0);
	ret.put("enfranchise.access","");
	ret.put("enfranchise.mode","");
	ret.put("qualify.age",900);
	ret.put("qualify.lines",0);
	ret.put("qualify.access","");
	ret.put("ballot.ack.chan",0);
	ret.put("ballot.ack.priv",1);
	ret.put("ballot.rej.chan",0);
	ret.put("ballot.rej.priv",1);
	ret.put("result.ack.chan",1);
	ret.put("visible.motion",1);
	ret.put("visible.ballots",0);
	ret.put("visible.active",1);
	ret.put("visible.veto",1);
	ret.put("weight.yea",0);
	ret.put("weight.nay",0);

	ret.merge(chan.get("config.vote"));                      // Overwrite defaults with saved config
	chan.set("config.vote",ret);                             // Write back combined result to db
	ret.merge(ret.get_child(type,Adoc{}));                   // Import type-specifc overrides up to main

	// Parse and validate any vote-time "audibles" from user.
	const Adoc auds(Adoc::arg_ctor,issue,ARG_KEYED,ARG_VALUED);
	const Adoc val_auds(ret.get_child("audibles",Adoc{}));
	const auto val_auds_set(val_auds.into<std::set<std::string>>());
	auds.for_each([&val_auds_set](const auto &key, const auto &val)
	{
		if(!val_auds_set.count(key))
			throw Exception("You cannot specify that option at vote-time.");
	});

	ret.merge(auds);
	ret.merge(cfg);                                          // Any overrides trumping all.
	return ret;
}()),
began(0),
ended(0),
expiry(0),
quorum(0)
{
	if(disabled())
		throw Exception("Votes of this type are disabled by the configuration.");
}


Vote::Vote(const std::string &type,
           const id_t &id,
           Adb &adb)
try:
Acct(&this->id,&adb),
id(lex_cast(id)),
type(get_val("type")),
chan(get_val("chan")),
nick(get_val("nick")),
acct(tolower(get_val("acct"))),
issue(get_val("issue")),
cfg(get("cfg")),
began(secs_cast(get_val("began"))),
ended(secs_cast(get_val("ended"))),
expiry(secs_cast(get_val("expiry"))),
quorum(has("quorum")? get_val<uint>("quorum") : 0),
reason(get_val("reason")),
effect(get_val("effect")),
yea(get("yea").into<decltype(yea)>()),
nay(get("nay").into<decltype(nay)>()),
veto(get("veto").into<decltype(veto)>()),
hosts(get("hosts").into<decltype(hosts)>())
{
	if(cfg.empty())
		throw Assertive("The configuration for this vote is missing and required.");
}
catch(const std::exception &e)
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
	doc.put("expiry",get_expiry());
	doc.put("quorum",get_quorum());
	doc.put("reason",get_reason());
	doc.put("effect",get_effect());
	doc.put_child("cfg",get_cfg());
	doc.put_child("yea",yea);
	doc.put_child("nay",nay);
	doc.put_child("veto",veto);
	doc.put_child("hosts",hosts);

	return doc;
}


void Vote::expire()
{
	expired();
	set_expiry();
	save();
}


void Vote::cancel()
{
	set_ended();
	set_reason("canceled");
	const scope s([this]
	{
		save();
	});

	if(total() >= cfg.get("visible.motion",1U))
		announce_canceled();

	if(!get_effect().empty())
		expired();

	canceled();
}


void Vote::start()
try
{
	using namespace colors;

	const auto &cfg(get_cfg());
	const auto &chan(get_chan());
	const auto &user(get_user());
	const auto speaker_ballot(cfg.get("speaker.ballot","y"));

	if(!get_quorum())
		set_quorum(calc_quorum(cfg,chan));

	if(is_ballot(speaker_ballot))
		cast(ballot(speaker_ballot),user);

	starting();
	set_began();
	save();

	if(cfg.get("visible.motion",1U) == 1U)
		announce_starting();
}
catch(...)
{
	if(!get_effect().empty())
		expired();

	throw;
}


void Vote::finish()
try
{
	const scope s([this]
	{
		if(!std::current_exception())
			save();
	});

	if(!get_ended())
		set_ended();

	if(interceded())
	{
		set_reason("vetoed");
		announce_vetoed();

		if(!get_effect().empty())
			expired();

		vetoed();
		return;
	}

	if(total() < get_quorum())
	{
		set_reason("quorum");
		if(total() >= cfg.get("visible.motion",1U))
			announce_failed_quorum();

		if(!get_effect().empty())
			expired();

		failed();
		return;
	}

	if(yea.size() < calc_required(cfg,tally()))
	{
		set_reason("plurality");
		if(total() >= cfg.get("visible.motion",1U))
			announce_failed_required();

		if(!get_effect().empty())
			expired();

		failed();
		return;
	}

	if(secs_cast(cfg["for"]) > 0)
	{
		// Adjust the final "for" time value using the weighting system
		const time_t min(secs_cast(cfg["for"]));
		const time_t add(secs_cast(cfg["weight.yea"]) * this->yea.size());
		const time_t sub(secs_cast(cfg["weight.nay"]) * this->nay.size());
		const time_t val(min + add - sub);
		cfg.put("for",val);
	}

	set_reason("");
	announce_passed();

	if(get_effect().empty())
		effective();

	passed();
}
catch(const std::exception &e)
{
	set_reason("error");
	if(!get_effect().empty())
		expired();

	save();

	if(cfg.get("result.ack.chan",true))
	{
		auto &chan(get_chan());
		chan << "The vote " << (*this) << " was rejected: " << e.what() << chan.flush;
	}
}


void Vote::event_vote(User &user,
                      const Ballot &ballot)
try
{
	const auto stat(cast(ballot,user));
	announce_ballot_accept(user,stat);

	if(stat == Stat::ADDED)
	{
		hosts.emplace(user.get_host());
		if(total() > 1 && cfg.get("visible.motion",1U) == total())
			announce_starting();
	}

	if(prejudiced() && get_effect().empty())
		effective();

	if(cfg.get("quorum.quick",false) && total() >= get_quorum() && yea.size() >= calc_required(cfg,tally()))
		set_ended();

	save();
}
catch(const Exception &e)
{
	announce_ballot_reject(user,string(e));
}


Stat Vote::cast(const Ballot &ballot,
                const User &user)
{
	if(get_ended())
		throw Exception("Vote has already ended.");

	const auto &cfg(get_cfg());
	const auto &chan(get_chan());
	const auto &began(get_began());
	const auto &acct(user.get_acct());

	if(!voted(user))
	{
		if(voted_host(user.get_host()) > 0)
			throw Exception("You can not cast another vote from this hostname.");

		if(!enfranchised(cfg,chan,user,began))
			throw Exception("You are not enfranchised for this vote.");

		if(!qualified(cfg,chan,user,began))
			throw Exception("You have not been active enough qualify for this vote.");
	}

	if(ballot == Ballot::NAY && intercession(cfg,chan,user))
		veto.emplace(user.get_acct());

	switch(ballot)
	{
		case Ballot::YEA:
			return !yea.emplace(user.get_acct()).second?  throw Exception("You have already voted yea."):
			       nay.erase(user.get_acct())?            Stat::CHANGED:
			                                              Stat::ADDED;
		case Ballot::NAY:
			return !nay.emplace(user.get_acct()).second?  throw Exception("You have already voted nay."):
			       yea.erase(user.get_acct())?            Stat::CHANGED:
			                                              Stat::ADDED;
		default:
			throw Exception("Ballot type not accepted.");
	}
}


void Vote::announce_starting()
{
	using namespace colors;

	auto &chan(get_chan());
	chan << "Vote " << (*this) << ": "
	     << BOLD << get_type() << OFF << ": " << UNDER2 << get_issue() << OFF << ". "
	     << "You have " << BOLD << secs_cast(secs_cast(cfg["duration"])) << OFF << " to vote; "
	     << BOLD << get_quorum() << OFF << " votes are required for a quorum! ";

	const auto &cfg(get_cfg());
	if(cfg.get("quorum.prejudice",false))
		chan << "Effects applied with prejudice. ";

	const auto vreq(cfg.get("veto.quorum",0));
	if(vreq > 1)
		chan << vreq << " vetoes are required to annul. ";

	chan << "Type or PM: "
	     << BOLD << FG::GREEN << "!vote y" << OFF << " " << BOLD << get_id() << OFF
	     << " or "
	     << BOLD << FG::RED << "!vote n" << OFF << " " << BOLD << get_id() << OFF
	     << chan.flush;
}


void Vote::announce_passed()
{
	using namespace colors;

	auto &chan(get_chan());
	if(cfg.get("result.ack.chan",true))
	{
		chan << (*this) << ": "
		     << BOLD << get_type() << OFF << ": "
		     << UNDER2 << get_issue() << OFF << ". "
		     << FG::WHITE << BG::GREEN << BOLD << "The yeas have it." << OFF
		     << " Yeas: " << FG::GREEN << BOLD << yea.size() << OFF << "."
		     << " Nays: " << FG::RED << nay.size() << OFF << ".";

		if(secs_cast(cfg["for"]) > 0)
			chan << " Effective for " << BOLD << secs_cast(secs_cast(cfg["for"])) << OFF << ".";

		chan << chan.flush;
	}
}


void Vote::announce_vetoed()
{
	using namespace colors;

	auto &chan(get_chan());
	if(cfg.get("result.ack.chan",true))
		chan << "The vote " << (*this) << " has been vetoed." << chan.flush;
}


void Vote::announce_canceled()
{
	using namespace colors;

	auto &chan(get_chan());
	if(cfg.get("result.ack.chan",true))
		chan << "The vote " << (*this) << " has been canceled." << chan.flush;
}


void Vote::announce_failed_required()
{
	using namespace colors;

	auto &chan(get_chan());
	if(cfg.get("result.ack.chan",true))
		chan << (*this) << ": "
		     << BOLD << get_type() << OFF << ": "
		     << UNDER2 << get_issue() << OFF << ". "
		     << FG::WHITE << BG::RED << BOLD << "The nays have it." << OFF
		     << " Yeas: " << FG::GREEN << yea.size() << OFF << "."
		     << " Nays: " << FG::RED << BOLD << nay.size() << OFF << "."
		     << " Required at least: " << BOLD << calc_required(cfg,tally()) << OFF << " yeas."
		     << chan.flush;
}


void Vote::announce_failed_quorum()
{
	using namespace colors;

	auto &chan(get_chan());
	if(cfg.get("result.ack.chan",true))
		chan << (*this) << ": "
		     << "Failed to reach a quorum: "
		     << BOLD << total() << OFF
		     << " of "
		     << BOLD << get_quorum() << OFF
		     << " required."
		     << chan.flush;
}


void Vote::announce_ballot_accept(User &user,
                                  const Stat &stat)
{
	const auto &cfg(get_cfg());
	switch(stat)
	{
		case Stat::ADDED:
		{
			if(cfg.get("ballot.ack.chan",false))
			{
				auto &chan(get_chan());
				chan << user << "Thanks for casting your vote on " << (*this) << "!" << chan.flush;
			}

			if(cfg.get("ballot.ack.priv",true))
				user << "Thanks for casting your vote on " << (*this) << "!" << user.flush;

			break;
		}

		case Stat::CHANGED:
		{
			if(cfg.get("ballot.ack.chan",false))
			{
				auto &chan(get_chan());
				chan << user << "You have changed your vote on " << (*this) << "!" << chan.flush;
			}

			if(cfg.get("ballot.ack.priv",true))
				user << "You have changed your vote on " << (*this) << "!" << user.flush;

			break;
		}
	}
}


void Vote::announce_ballot_reject(User &user,
                                  const std::string &reason)
{
	const auto &cfg(get_cfg());
	if(cfg.get("ballot.rej.chan",false))
	{
		auto &chan(get_chan());
		chan << user << "Your vote was not accepted for " << (*this) << ": " << reason << chan.flush;
	}

	if(cfg.get("ballot.rej.priv",true))
		user << "Your vote was not accepted for " << (*this) << ": " << reason << user.flush;
}


bool Vote::voted(const User &user)
const
{
	return voted_acct(user.get_acct()) ||
	       voted_host(user.get_host());
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


Ballot Vote::position(const std::string &acct)
const
{
	return yea.count(acct)? Ballot::YEA:
	       nay.count(acct)? Ballot::NAY:
	                        throw Exception("No position taken.");
}


bool Vote::prejudiced()
const
{
	const Adoc &cfg(get_cfg());
	if(!cfg.get("quorum.prejudice",false))
		return false;

	return yea.size() >= calc_required(cfg,tally());
}


bool Vote::interceded()
const
{
	const auto vmin(std::max(cfg.get("veto.quorum",1U),1U));
	const auto num_vetoes(get_veto().size());
	if(num_vetoes < vmin)
		return false;

	const auto quick(cfg.get("veto.quick",true));
	return quick? true : !remaining();
}


Locutor &operator<<(Locutor &locutor,
                    const Vote &vote)
{
	using namespace colors;

	return locutor << "#" << BOLD << vote.get_id() << OFF;
}


uint calc_required(const Adoc &cfg,
                   const Tally &tally)
{
	const std::vector<uint> sel
	{{
		calc_plurality(cfg,tally),
		cfg.get("quorum.yea",0U)
	}};

	return *std::max_element(sel.begin(),sel.end());
}


uint calc_plurality(const Adoc &cfg,
                    const Tally &tally)
{
	const auto &yea(tally.first);
	const auto &nay(tally.second);
	const auto total(yea + nay);
	const float count((total - yea) + nay);
	return ceil(count * cfg.get("quorum.plurality",0.51));
}


uint calc_quorum(const Adoc &cfg,
                 const Chan &chan,
                 time_t began)
{
	std::vector<uint> sel
	{{
		cfg.get("quorum.yea",1U),
		cfg.get("quorum.ballots",1U),
		0
	}};

	const auto turnout(cfg.get("quorum.turnout",0.0));
	if(turnout <= 0.0)
		return *std::max_element(sel.begin(),sel.end());

	if(!began)
		time(&began);

	std::map<std::string,uint> count;
	chan.users.for_each([&cfg,&chan,&began,&count]
	(const User &user)
	{
		if(enfranchised(cfg,chan,user,began))
			count.emplace(user.get_acct(),0);
	});

	const auto min_age(secs_cast(cfg["quorum.age"]));
	const auto start_time(began - min_age);
	irc::log::for_each(chan.get_name(),[&count,&start_time]
	(const irc::log::ClosureArgs &a)
	{
		if(a.time < start_time)
			return true;

		if(strncmp(a.type,"PRI",3) != 0)  // PRIVMSG
			return true;

		if(strnlen(a.acct,16) == 0 || *a.acct == '*')
			return true;

		const auto it(count.find(a.acct));
		if(it != count.end())
		{
			auto &lines(it->second);
			++lines;
		}

		return true;
	});

	const auto min_lines(cfg.get("quorum.lines",0U));
	const auto has_lines(std::count_if(count.begin(),count.end(),[&min_lines]
	(const auto &p)
	{
		const auto &acct(p.first);
		const auto &lines(p.second);
		return lines >= min_lines;
	}));

	sel.back() = ceil(has_lines * turnout);
	return *std::max_element(sel.begin(),sel.end());
}


bool speaker(const Adoc &cfg,
             const Chan &chan,
             const User &user)
{
	if(cfg.get("disable",false))
		return false;

	if(!user.is_logged_in())
		return false;

	return (!cfg.has("speaker.access") || has_access(chan,user,cfg.get("speaker.access",Mode{}))) &&
	       (!cfg.has("speaker.mode") || has_mode(chan,user,cfg.get("speaker.mode",Mode{})));
}


bool intercession(const Adoc &cfg,
                  const Chan &chan,
                  const User &user)
{
	if(cfg.get("disable",false))
		return false;

	if(!user.is_logged_in())
		return false;

	if(cfg.has("veto.mode") && !has_mode(chan,user,cfg.get("veto.mode",Mode{})))
		return false;

	if(cfg.has("veto.access") && !has_access(chan,user,cfg.get("veto.access",Mode{})))
		return false;

	return cfg.has("veto.access") || cfg.has("veto.mode");
}


bool qualified(const Adoc &cfg,
               const Chan &chan,
               const User &user,
               time_t began)
{
	if(cfg.get("disable",false))
		return false;

	if(!user.is_logged_in())
		return false;

	if(has_access(chan,user,cfg.get("qualify.access",Mode{})))
		return true;

	if(!began)
		time(&began);

	const auto age(secs_cast(cfg.get("qualify.age","10m")));
	const auto &endtime(began - age);
	const auto &acct(user.get_acct());
	const irc::log::FilterAll filt([&acct,&endtime]
	(const irc::log::ClosureArgs &a)
	{
		if(a.time > endtime)
			return false;

		if(strncmp(a.type,"PRI",3) != 0)  // PRIVMSG
			return false;

		return strncmp(a.acct,acct.c_str(),16) == 0;
	});

	const auto lines(cfg.get("qualify.lines",0U));
	return irc::log::atleast(chan.get_name(),filt,lines);
}


bool enfranchised(const Adoc &cfg,
                  const Chan &chan,
                  const User &user,
                  time_t began)
{
	if(cfg.get("disable",false))
		return false;

	if(!user.is_logged_in())
		return false;

	const auto mode(cfg.get("enfranchise.mode",Mode{}));
	const auto access(cfg.get("enfranchise.access",Mode{}));
	if(!mode.empty() || !access.empty())
		return has_mode(chan,user,mode) || has_access(chan,user,access);

	if(!began)
		time(&began);

	const auto age(secs_cast(cfg.get("enfranchise.age","30m")));
	const auto endtime(began - age);
	const auto &acct(user.get_acct());
	const irc::log::FilterAll filt([&acct,&endtime]
	(const irc::log::ClosureArgs &a)
	{
		if(a.time > endtime)
			return false;

		if(strncmp(a.type,"PRI",3) != 0)  // PRIVMSG
			return false;

		return strncmp(a.acct,acct.c_str(),16) == 0;
	});

	const auto lines(cfg.get("enfranchise.lines",0U));
	return irc::log::atleast(chan.get_name(),filt,lines);
}


bool has_mode(const Chan &chan,
              const User &user,
              const Mode &mode)
try
{
	return chan.users.mode(user).any(mode);
}
catch(const std::out_of_range &e)
{
	return false;
}


bool has_access(const Chan &chan,
                const User &user,
                const Mode &flags)
{
	if(!chan.lists.has_flag(user))
		return false;

	const auto &f(chan.lists.get_flag(user));
	return f.get_flags().any(flags);
}


std::ostream &operator<<(std::ostream &s,
                         const Ballot &ballot)
{
	switch(ballot)
	{
		case Ballot::YEA:    s << "YEA";     break;
		case Ballot::NAY:    s << "NAY";     break;
	}

	return s;
}


Ballot ballot(const std::string &str)
{
	if(ystr.count(str))
		return Ballot::YEA;

	if(nstr.count(str))
		return Ballot::NAY;

	throw Exception("Supplied string is not a valid Ballot");
}


bool is_ballot(const std::string &str)
{
	return ystr.count(str) || nstr.count(str);
}
