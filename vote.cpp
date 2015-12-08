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


decltype(Vote::ystr) Vote::ystr                  { "yea", "yes", "yea", "Y", "y"                   };
decltype(Vote::nstr) Vote::nstr                  { "nay", "no", "N", "n"                           };
decltype(Vote::ARG_KEYED) Vote::ARG_KEYED        { "--"                                            };
decltype(Vote::ARG_VALUED) Vote::ARG_VALUED      { "="                                             };


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
	ret.put("audibles","");
	ret.put("limit.active",16);
	ret.put("limit.user",1);
	ret.put("quorum.ballots",1);
	ret.put("quorum.yea",1);
	ret.put("quorum.age",0);
	ret.put("quorum.lines",0);
	ret.put("quorum.turnout",0.00);
	ret.put("quorum.plurality",0.51);
	ret.put("quorum.quick",0);
	ret.put("motion.quorum",1);
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
	ret.put("visible.ballots",0);
	ret.put("visible.veto",1);
	ret.put("weight.yea",0);
	ret.put("weight.nay",0);

	ret.merge(chan.get("config.vote"));                      // Overwrite defaults with saved config
	chan.set("config.vote",ret);                             // Write back combined result to db
	ret.merge(ret.get_child(type,Adoc()));                   // Import type-specifc overrides up to main

	// Parse and validate any vote-time "audibles" from user.
	const Adoc auds(Adoc::arg_ctor,issue,ARG_KEYED,ARG_VALUED);
	const Adoc val_auds(ret.get_child("audibles",Adoc()));
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

	// Initiation implies yes vote, except if initiated by the bot
	if(!user.is_myself())
	{
		yea.emplace(get_user_acct());
		hosts.emplace(user.get_host());
	}
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
	const scope s([&]{ save(); });
	if(total() >= cfg.get<uint>("motion.quorum"))
		announce_canceled();

	canceled();
}


void Vote::start()
{
	using namespace colors;

	if(!get_quorum())
		set_quorum(calc_quorum());

	starting();
	set_began();
	save();

	if(cfg.get<uint>("motion.quorum") == 1)
		announce_starting();
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
		vetoed();
		return;
	}

	if(total() < get_quorum())
	{
		set_reason("quorum");
		if(total() >= cfg.get<uint>("motion.quorum"))
			announce_failed_quorum();

		failed();
		return;
	}

	if(yea.size() < required())
	{
		set_reason("plurality");
		if(total() >= cfg.get<uint>("motion.quorum"))
			announce_failed_required();

		failed();
		return;
	}

	if(cfg.get<time_t>("for") > 0)
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
	passed();
}
catch(const std::exception &e)
{
	set_reason(e.what());
	save();

	if(cfg.get<bool>("result.ack.chan"))
	{
		auto &chan = get_chan();
		chan << "The vote " << (*this) << " was rejected: " << e.what() << flush;
	}
}


void Vote::valid(const Adoc &cfg)
const
{
	const auto valid_sentence = [](const auto &val)
	{
		const auto isalpha_or_sp = [&](auto&& c)
		{
			return std::isalpha(c,locale) || c == ' ';
		};

		if(!std::all_of(val.begin(),val.end(),isalpha_or_sp))
			throw Exception("Must use letters only as value for this key.");
	};

	const auto valid_num = [](const auto &val)
	{
		if(!std::all_of(val.begin(),val.end(),[](auto&& c) { return isdigit(c) || c == '.'; }))
			throw Exception("Must use a numerical value for this key.");

		if(std::count(val.begin(),val.end(),'.') > 1)
			throw Exception("One dot only, please.");
	};

	const auto valid_mode = [](const auto &val)
	{
		if(!isalpha(val))
			throw Exception("Must use letters only as value for this key.");
	};

	const auto valid_secs = [](const auto &val)
	{
		secs_cast(val);  // throws if invalid
	};

	const auto chk = [&](const auto &key, const auto &val)
	{
		static const auto sentence_keys =
		{
			"audibles",
		};

		static const auto mode_keys =
		{
			"access",
			"mode",
		};

		static const auto secs_keys =
		{
			"for",
			"age",
			"duration",
		};

		if(endswith_any(key,sentence_keys.begin(),sentence_keys.end()))
			valid_sentence(val);
		else if(endswith_any(key,mode_keys.begin(),mode_keys.end()))
			valid_mode(val);
		else if(endswith_any(key,secs_keys.begin(),secs_keys.end()))
			valid_secs(val);
		else
			valid_num(val);
	};

	cfg.for_each(chk);
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
		{
			hosts.emplace(user.get_host());

			if(cfg.get<bool>("ballot.ack.chan"))
				chan << user << "Thanks for casting your vote on " << (*this) << "!" << flush;

			if(cfg.get<bool>("ballot.ack.priv"))
				user << "Thanks for casting your vote on " << (*this) << "!" << flush;

			if(total() > 1 && cfg.get<uint>("motion.quorum") == total())
				announce_starting();

			break;
		}

		case CHANGED:
		{
			if(cfg.get<bool>("ballot.ack.chan"))
				chan << user << "You have changed your vote on " << (*this) << "!" << flush;

			if(cfg.get<bool>("ballot.ack.priv"))
				user << "You have changed your vote on " << (*this) << "!" << flush;

			break;
		}
	}

	if(cfg.get<bool>("quorum.quick",0) && total() >= get_quorum() && yea.size() >= required())
		set_ended();

	save();
}
catch(const Exception &e)
{
	using namespace colors;

	if(cfg.get<bool>("ballot.rej.chan"))
	{
		Chan &chan = get_chan();
		chan << user << "Your vote was not accepted for " << (*this) << ": " << e << flush;
	}

	if(cfg.get<bool>("ballot.rej.priv"))
		user << "Your vote was not accepted for " << (*this) << ": " << e << flush;
}


Vote::Stat Vote::cast(const Ballot &ballot,
                      const User &user)
{
	if(get_ended())
		throw Exception("Vote has already ended.");

	if(!voted(user))
	{
		if(voted_host(user.get_host()) > 0)
			throw Exception("You can not cast another vote from this hostname.");

		if(!enfranchised(user))
			throw Exception("You are not yet enfranchised in this channel.");

		if(!qualified(user))
			throw Exception("You have not been active enough qualify for this vote.");
	}

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


void Vote::announce_starting()
{
	using namespace colors;

	auto &chan(get_chan());
	chan << "Vote " << (*this) << ": "
	     << BOLD << get_type() << OFF << ": " << UNDER2 << get_issue() << OFF << ". "
	     << "You have " << BOLD << secs_cast(secs_cast(cfg["duration"])) << OFF << " to vote; "
	     << BOLD << get_quorum() << OFF << " votes are required for a quorum! "
	     << "Type or PM: "
	     << BOLD << FG::GREEN << "!vote y" << OFF << " " << BOLD << get_id() << OFF
	     << " or "
	     << BOLD << FG::RED << "!vote n" << OFF << " " << BOLD << get_id() << OFF
	     << flush;
}


void Vote::announce_passed()
{
	using namespace colors;

	auto &chan(get_chan());
	if(cfg.get<bool>("result.ack.chan"))
	{
		chan << (*this) << ": "
		     << BOLD << get_type() << OFF << ": "
		     << UNDER2 << get_issue() << OFF << ". "
		     << FG::WHITE << BG::GREEN << BOLD << "The yeas have it." << OFF
		     << " Yeas: " << FG::GREEN << BOLD << yea.size() << OFF << "."
		     << " Nays: " << FG::RED << nay.size() << OFF << ".";

		if(cfg.get<time_t>("for") > 0)
			chan << " Effective for " << BOLD << secs_cast(cfg.get<time_t>("for")) << OFF << ".";

		chan << flush;
	}
}


void Vote::announce_vetoed()
{
	using namespace colors;

	auto &chan(get_chan());
	if(cfg.get<bool>("result.ack.chan"))
		chan << "The vote " << (*this) << " has been vetoed." << flush;
}


void Vote::announce_canceled()
{
	using namespace colors;

	auto &chan(get_chan());
	if(cfg.get<bool>("result.ack.chan"))
		chan << "The vote " << (*this) << " has been canceled." << flush;
}


void Vote::announce_failed_required()
{
	using namespace colors;

	auto &chan(get_chan());
	if(cfg.get<bool>("result.ack.chan"))
		chan << (*this) << ": "
		     << BOLD << get_type() << OFF << ": "
		     << UNDER2 << get_issue() << OFF << ". "
		     << FG::WHITE << BG::RED << BOLD << "The nays have it." << OFF
		     << " Yeas: " << FG::GREEN << yea.size() << OFF << "."
		     << " Nays: " << FG::RED << BOLD << nay.size() << OFF << "."
		     << " Required at least: " << BOLD << required() << OFF << " yeas."
		     << flush;
}


void Vote::announce_failed_quorum()
{
	using namespace colors;

	auto &chan(get_chan());
	if(cfg.get<bool>("result.ack.chan"))
		chan << (*this) << ": "
		     << "Failed to reach a quorum: "
		     << BOLD << total() << OFF
		     << " of "
		     << BOLD << get_quorum() << OFF
		     << " required."
		     << flush;
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
	filt.time.first = get_began() - secs_cast(cfg["qualify.age"]);
	filt.time.second = get_began();
	filt.type = "PRI";  // PRIVMSG

	const Logs &logs(get_logs());
	return logs.atleast(get_chan_name(),filt,cfg.get<uint>("qualify.lines"));
}


bool Vote::enfranchised(const User &user)
const
{
	const Adoc &cfg = get_cfg();
	if(cfg.has("enfranchise.access") || cfg.has("enfranchise.mode"))
		return has_mode(user,cfg["enfranchise.mode"]) ||
		       has_access(user,cfg["enfranchise.access"]);

	Logs::SimpleFilter filt;
	filt.acct = user.get_acct();
	filt.time.first = 0;
	filt.time.second = get_began() - secs_cast(cfg["enfranchise.age"]);
	filt.type = "PRI";  // PRIVMSG

	const Logs &logs(get_logs());
	return logs.atleast(get_chan_name(),filt,cfg.get<uint>("enfranchise.lines"));
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


Vote::Ballot Vote::ballot(const std::string &str)
{
	if(ystr.count(str))
		return Ballot::YEA;

	if(nstr.count(str))
		return Ballot::NAY;

	throw Exception("Supplied string is not a valid Ballot");
}


bool Vote::is_ballot(const std::string &str)
{
	return ystr.count(str) || nstr.count(str);
}


uint Vote::required()
const
{
	const std::vector<uint> sel
	{{
		plurality(),
		cfg.get<uint>("quorum.yea",0)
	}};

	return *std::max_element(sel.begin(),sel.end());
}


uint Vote::plurality()
const
{
	const float &total(this->total());
	const float count((total - yea.size()) + nay.size());
	return ceil(count * cfg.get<float>("quorum.plurality"));
}


bool Vote::interceded()
const
{
	const auto vmin = std::max(cfg.get<uint>("veto.quorum"),1U);
	if(num_vetoes() < vmin)
		return false;

	const auto quick = cfg.get<bool>("veto.quick");
	return quick? true : !remaining();
}


uint Vote::calc_quorum()
const
{
	const Adoc &cfg(get_cfg());
	const Chan &chan(get_chan());
	const Logs &logs(get_logs());

	std::vector<uint> sel
	{{
		cfg.get<uint>("quorum.yea"),
		cfg.get<uint>("quorum.ballots"),
		0
	}};

	const auto turnout(cfg.get<float>("quorum.turnout"));
	if(turnout <= 0.0)
		return *std::max_element(sel.begin(),sel.end());

    std::map<std::string,uint> count;
	chan.users.for_each([this,&count]
	(const User &user)
	{
		if(enfranchised(user))
			count.emplace(user.get_acct(),0);
	});

	Logs::SimpleFilter filt;
	const auto curtime(time(nullptr));
	const auto min_age(secs_cast(cfg["quorum.age"]));
	filt.time.first = curtime - min_age;
	filt.time.second = curtime;
	filt.type = "PRI";  // PRIVMSG
	logs.for_each(get_chan_name(),filt,[&count]
	(const Logs::ClosureArgs &a)
	{
		const auto it(count.find(a.acct));
		if(it == count.end())
			return true;

		uint &lines(it->second);
		++lines;
		return true;
	});

	const auto min_lines(cfg.get<uint>("quorum.lines",0U));
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


Locutor &operator<<(Locutor &locutor,
                    const Vote &vote)
{
	using namespace colors;

	return locutor << "#" << BOLD << vote.get_id() << OFF;
}
