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
#include "vdb.h"
#include "praetor.h"
#include "voting.h"


Voting::Voting(Bot &bot,
               Vdb &vdb,
               Praetor &praetor):
bot(bot),
vdb(vdb),
praetor(praetor),
interrupted(false),
initialized(false),
poll_thread(&Voting::poll_worker,this),
remind_thread(&Voting::remind_worker,this),
eligible_thread(&Voting::eligible_worker,this)
{
}


Voting::~Voting() noexcept
{
	interrupted.store(true,std::memory_order_release);
	sem.notify_all();
	eligible_thread.join();
	remind_thread.join();
	poll_thread.join();
}


void Voting::cancel(const id_t &id,
                    const Chan &chan,
                    const User &user)
{
	Vote &vote(get(id));
	cancel(vote,chan,user);
}


void Voting::cancel(Vote &vote,
                    const Chan &chan,
                    const User &user)
{
	if(user.get_acct() != vote.get_user_acct())
		throw Exception() << "You can't cancel a vote by " << vote.get_user_acct() << ".";

	if(vote.total() > 1)
		throw Exception("You can't cancel after someone else has voted.");

	const auto &cfg(vote.get_cfg());
	if(!cfg.get("cancel",true))
		throw Exception("You can't cancel votes of this type.");

	vote.cancel();
	del(vote.get_id());
}


void Voting::valid_motion(Vote &vote)
{
	const auto &cfg(vote.get_cfg());
	const auto &user(vote.get_user());
	const auto &chan(vote.get_chan());
	valid_limits(vote,chan,user);

	if(!speaker(cfg,chan,user))
		throw Exception("You are not able to create votes on this channel.");

	if(!enfranchised(cfg,chan,user))
		throw Exception("You are not yet enfranchised in this channel.");

	if(!qualified(cfg,chan,user))
		throw Exception("You have not been participating enough to start a vote.");

	for_each(chan,[&vote](Vote &existing)
	{
		if(vote.get_id() == existing.get_id())
			return;

		if(vote.get_type() != existing.get_type())
			return;

		if(!boost::iequals(vote.get_issue(),existing.get_issue()))
			return;

		auto &user(vote.get_user());
		existing.event_vote(user,Ballot::YEA);
		throw Exception("Instead an attempt was made to cast a ballot for #") << existing.get_id() << ".";
	});

	const auto now(time(nullptr));
	const auto limit_age(secs_cast(cfg["limit.age"]));
	const auto limit_quorum(secs_cast(cfg["limit.quorum.age"]));
	const auto limit_plurality(secs_cast(cfg["limit.plurality.age"]));

	const Vdb::Terms age_query
	{
		Vdb::Term { "ended",  ">=", lex_cast(now - limit_age)        },
		Vdb::Term { "issue",  "==", vote.get_issue()                 },
		Vdb::Term { "type",   "==", vote.get_type()                  },
		Vdb::Term { "chan",   "==", chan.get_name()                  },
	};

	const Vdb::Terms quorum_query
	{
		Vdb::Term { "reason", "==", "quorum"                         },
		Vdb::Term { "ended",  ">=", lex_cast(now - limit_quorum)     },
		Vdb::Term { "issue",  "==", vote.get_issue()                 },
		Vdb::Term { "type",   "==", vote.get_type()                  },
		Vdb::Term { "chan",   "==", chan.get_name()                  },
	};

	const Vdb::Terms plurality_query
	{
		Vdb::Term { "reason", "==", "plurality"                      },
		Vdb::Term { "ended",  ">=", lex_cast(now - limit_plurality)  },
		Vdb::Term { "issue",  "==", vote.get_issue()                 },
		Vdb::Term { "type",   "==", vote.get_type()                  },
		Vdb::Term { "chan",   "==", chan.get_name()                  },
	};

	if(limit_age && !vdb.query(age_query,1).empty())
		throw Exception("This vote was made too recently. Try again later.");

	if(limit_quorum && !vdb.query(quorum_query,1).empty())
		throw Exception("This vote failed without a quorum too recently. Try again later.");

	if(limit_plurality && !vdb.query(plurality_query,1).empty())
		throw Exception("This vote failed with plurality too recently. Try again later.");
}


void Voting::valid_limits(Vote &vote,
                          const Chan &chan,
                          const User &user)
{
	using limits = std::numeric_limits<uint>;

	if(user.is_myself() || chan.users.mode(user).has('o'))
		return;

	const auto &cfg(vote.get_cfg());
	if(count(chan) > cfg.get("limit.active",limits::max()))
		throw Exception("Too many active votes for this channel.");

	if(count(chan,user) > cfg.get("limit.user",limits::max()))
		throw Exception("Too many active votes started by you on this channel.");

	auto typcnt(0);
	for_each(chan,user,[&typcnt,&vote]
	(const Vote &active)
	{
		if(vote.get_type() == "appeal")
			return;

		typcnt += active.get_type() == vote.get_type();
	});

	if(typcnt > cfg.get("limit.type",1))
		throw Exception("Too many active votes of this type started by you on this channel.");
}


void Voting::eligible_worker()
{
	worker_wait_init();
	while(!interrupted.load(std::memory_order_consume)) try
	{
		eligible_add();
		worker_sleep(std::chrono::hours(12));
	}
	catch(const Internal &e)
	{
		std::cerr << "[Voting (eligible worker)]: \033[1;41m" << e << "\033[0m" << std::endl;
	}
}


void Voting::eligible_add()
{
	const std::lock_guard<Bot> lock(bot);
	auto &chans(get_chans());
	chans.for_each([this](Chan &chan)
	{
		eligible_add(chan);
	});
}


void Voting::eligible_add(Chan &chan)
try
{
	const auto civis(chan.get("config.vote.civis"));
	const auto age(secs_cast(civis["eligible.age"]));
	const auto lines(civis.get<uint>("eligible.lines",0));
	const auto automat(civis.get<bool>("eligible.automatic",0));
	const Adoc excludes(civis.get_child("eligible.exclude",Adoc{}));
	const auto exclude(excludes.into<std::set<std::string>>());
	if(!lines || !age || !automat)
		return;

	std::cout << "Finding eligible for channel " << chan.get_name() << std::endl;

	std::map<std::string,uint> count;
	std::map<std::string,std::string> accts;
	const auto endtime(time(NULL) - age);
	irc::log::for_each(chan.get_name(),[&count,&accts,&endtime]
	(const irc::log::ClosureArgs &a)
	{
		if(a.time > endtime)
			return true;

		if(strncmp(a.type,"PRI",3) != 0)  // PRIVMSG
			return true;

		if(strnlen(a.acct,16) == 0 || *a.acct == '*')
			return true;

		++count[a.acct];
		const auto iit(accts.emplace(a.acct,a.nick));
		if(iit.second)
			return true;

		auto &nick(iit.first->second);
		if(nick != a.nick)
			nick = a.nick;

		return true;
	});

	for(const auto &p : count) try
	{
		const auto &acct(p.first);
		if(exclude.count(acct))
			continue;

		const auto &linecnt(p.second);
		if(linecnt < lines)
			continue;

		auto &users(get_users());
		const auto &nick(accts.at(acct));
		if(!users.has(nick))
			continue;

		User &user(users.get(nick));
		if(!user.is_logged_in())
			continue;

		if(chan.lists.has_flag(user,'V'))
			continue;

		bool exists(false);
		for_each([&exists,&acct](const Vote &vote)
		{
			if(vote.get_type() == "civis" && vote.get_issue() == acct)
				exists = true;
		});

		if(exists)
			continue;

		const auto ids =
		{
			eligible_last_vote(chan,nick,
			{
				Vdb::Term { "type",   "==", "quiet"      },
				Vdb::Term { "reason", "==", ""           },
			}),

			eligible_last_vote(chan,nick,
			{
				Vdb::Term { "type",   "==", "ban"        },
				Vdb::Term { "reason", "==", ""           },
			}),

			eligible_last_vote(chan,acct,
			{
				Vdb::Term { "type",   "==", "civis"      },
				Vdb::Term { "reason", "==", "plurality"  },
			}),
		};

		const auto &id(*std::max_element(std::begin(ids),std::end(ids)));
		if(id)
		{
			const auto startfrom(lex_cast<time_t>(vdb.get_value(id,"ended")));
			const irc::log::FilterAny filt([&acct,&startfrom]
			(const irc::log::ClosureArgs &a)
			{
				if(a.time < startfrom)
					return false;

				if(strncmp(a.type,"PRI",3) != 0)   // PRIVMSG
					return false;

				return strncmp(a.acct,acct.c_str(),16) == 0;
			});

			if(irc::log::count(chan.get_name(),filt) < lines)
				continue;
		}

		const auto &sess(get_sess());
		auto &myself(users.get(sess.get_nick()));
		const auto &vote(motion<vote::Civis>(chan,myself,nick));
		user << "You are now eligible to be a citizen of " << chan.get_name() << "! ";
		user << " Remind people to vote for issue #" << vote.get_id() << "!";
		user << user.flush;
	}
	catch(const std::exception &e)
	{
		std::cerr << "Error on: [" << p.first << " : " << p.second << "]: " << e.what() << std::endl;
	}

	std::cout << "Finished eligible for channel " << chan.get_name() << std::endl;
}
catch(const std::exception &e)
{
	std::cerr << "Voting::eligible_add(" << chan.get_name() << "): " << e.what() << std::endl;
}


id_t Voting::eligible_last_vote(const Chan &chan,
                                const std::string &nick,
                                Vdb::Terms terms)
{
	terms.emplace_front(Vdb::Term{"issue","==",nick});
	terms.emplace_front(Vdb::Term{"chan","==",chan.get_name()});

	const auto res(vdb.query(terms,1));
	return !res.empty()? res.front() : 0;   // note sentinel 0 is a valid vote id but ignored here
}


void Voting::remind_worker()
{
	worker_wait_init();
	while(!interrupted.load(std::memory_order_consume)) try
	{
		remind_votes();
		worker_sleep(std::chrono::hours(24));
	}
	catch(const Internal &e)
	{
		std::cerr << "[Voting (remind worker)]: \033[1;41m" << e << "\033[0m" << std::endl;
	}
}


void Voting::remind_votes()
{
	using namespace colors;

	const std::unique_lock<Bot> lock(bot);
	static const milliseconds delay(750);
	//const FloodGuard guard(bot.sess,delay);

	const auto &chans(get_chans());
	for(auto it(votes.cbegin()); it != votes.cend(); ++it)
	{
		const auto &vote(*it->second);
		if(!chans.has(vote.get_chan_name()))
			continue;

		const auto &cfg(vote.get_cfg());
		if(!cfg.get("remind.enable",false))
			continue;

		auto &chan(vote.get_chan());
		chan.users.for_each([&](User &user)
		{
			if(vote.voted(user))
				return;

			if(!enfranchised(cfg,chan,user))
				return;

			user << user.PRIVMSG << chan << user.get_nick() << ", I see you have not yet voted on issue "
			     << vote << ", " << BOLD << vote.get_type() << OFF << ": " << UNDER2 << vote.get_issue() << OFF << ". "
			     << "Your participation as a citizen of " << chan.get_name() << " is highly valued to maintain a fair and democratic community. "
			     << "Please consider " << BOLD << FG::GREEN << "!v y " << vote.get_id() << OFF << " or " << BOLD << FG::RED << "!v n " << vote.get_id() << OFF
			     << " before the issue closes in " << BOLD << secs_cast(vote.remaining()) << OFF << ". "
			     << user.flush;

			// TODO: fix guard
			std::this_thread::sleep_for(delay);
		});
	}
}


void Voting::poll_worker()
{
	poll_init();
	initialized.store(true,std::memory_order_release);
	sem.notify_all();

	while(!interrupted.load(std::memory_order_consume)) try
	{
		poll_votes();
		worker_sleep(std::chrono::seconds(2));
	}
	catch(const std::exception &e)
	{
		std::cerr << "[Voting (poll worker)]: \033[1;41m"
		          << e.what() << "\033[0m"
		          << std::endl;
	}
}


void Voting::poll_init()
{
	const std::lock_guard<Bot> lock(bot);
	bot.set_tls_context();
	std::cout << "[Voting]: Adding previously open votes."
	          << " Reading " << vdb.count() << " votes..."
	          << std::endl;

	auto it(vdb.cbegin(stldb::SNAPSHOT));
	const auto end(vdb.cend());
	for(; it != end && !interrupted.load(std::memory_order_consume); ++it) try
	{
		const auto id(lex_cast<id_t>(it->first));
		const Adoc doc(it->second);
		const auto ended(secs_cast(doc["ended"]));
		if(!ended)
		{
			std::cout << "Adding open vote #" << id << std::endl;
			auto vote(vdb.get(id));
			const auto iit(votes.emplace(id,std::move(vote)));
			{
				const auto &vote(*iit.first->second);
				chanidx.emplace(vote.get_chan_name(),id);
				useridx.emplace(vote.get_user_acct(),id);
			}
		}
	}
	catch(const std::exception &e)
	{
		const auto id(lex_cast<id_t>(it->first));
		std::cerr << "[Voting]: Failed reading #" << id
		          << ": \033[1;31m" << e.what()
		          << "\033[0m" << std::endl;
	}
}


void Voting::poll_votes()
{
	const std::unique_lock<Bot> lock(bot);
	const auto &chans(get_chans());
	for(auto it(votes.begin()); it != votes.end();)
	{
		auto &vote(*it->second);
		const bool finished
		{
			vote.get_ended()       ||
			vote.remaining() <= 0  ||
			vote.interceded()
		};

		if(finished && chans.has(vote.get_chan_name()))
		{
			call_finish(vote);
			praetor.add(std::move(del(it++)));
		}
		else ++it;
	}
}


void Voting::call_finish(Vote &vote)
noexcept try
{
	vote.finish();
}
catch(const std::exception &e)
{
	std::stringstream err;
	err << "An internal error occurred in Vote::finish(): " << e.what() << ".";

	std::cerr << "[Voting]: \033[1;31m" << err.str() << "\033[0m" << std::endl;

	const auto &chans(get_chans());
	if(chans.has(vote.get_chan_name()))
	{
		Chan &chan(vote.get_chan());
		chan << err.str() << chan.flush;
	}
}


std::unique_ptr<Vote> Voting::del(const id_t &id)
{
	const auto vit = votes.find(id);
	if(vit == votes.end())
		throw Exception("Could not delete any vote by this ID.");

	return del(vit);
}


std::unique_ptr<Vote> Voting::del(const decltype(votes.begin()) &it)
{
	const id_t &id(it->first);
	auto vote(std::move(it->second));

	const auto deindex = [&id]
	(auto &map, const auto &ent)
	{
		const auto pit = map.equal_range(ent);
		const auto it = std::find_if(pit.first,pit.second,[&id]
		(const auto &it)
		{
			const id_t &idx = it.second;
			return idx == id;
		});

		if(it != pit.second)
			map.erase(it);
	};

	deindex(chanidx,vote->get_chan_name());
	deindex(useridx,vote->get_user_acct());
	votes.erase(it);

	return std::move(vote);
}


template<class Duration>
void Voting::worker_sleep(Duration&& duration)
{
	using std::cv_status;
	using std::chrono::steady_clock;

	const auto now(steady_clock::now());
	const auto fut(now + duration);
	std::unique_lock<decltype(mutex)> lock(mutex);
	while(sem.wait_until(lock,fut) != cv_status::timeout && !interrupted.load(std::memory_order_consume));
}


void Voting::worker_wait_init()
{
	{
		std::unique_lock<decltype(mutex)> lock(mutex);
		sem.wait(lock,[this]
		{
			return initialized.load(std::memory_order_consume) ||
			       interrupted.load(std::memory_order_consume);
		});
	}

	{
		const std::unique_lock<Bot> lock(bot);
		bot.set_tls_context();
	}
}


id_t Voting::get_next_id()
const
{
	id_t i(0);
	while(vdb.exists(i) || exists(i))
		i++;

	return i;
}


void Voting::for_each(const std::function<void (Vote &vote)> &closure)
{
	for(auto &p : votes)
		closure(*p.second);
}


void Voting::for_each(const User &user,
                      const std::function<void (Vote &vote)> &closure)
{
	auto pit(useridx.equal_range(user.get_acct()));
	for(; pit.first != pit.second; ++pit.first)
	{
		const auto &id(pit.first->second);
		closure(get(id));
	}
}


void Voting::for_each(const Chan &chan,
                      const std::function<void (Vote &vote)> &closure)
{
	auto pit(chanidx.equal_range(chan.get_name()));
	for(; pit.first != pit.second; ++pit.first)
	{
		const auto &id(pit.first->second);
		closure(get(id));
	}
}


void Voting::for_each(const Chan &chan,
                      const User &user,
                      const std::function<void (Vote &vote)> &closure)
{
	const auto ids(get_ids(chan,user));
	for(const auto &id : ids)
		closure(get(id));
}


void Voting::for_each(const std::function<void (const Vote &vote)> &closure)
const
{
	for(const auto &p : votes)
		closure(*p.second);
}


void Voting::for_each(const User &user,
                      const std::function<void (const Vote &vote)> &closure)
const
{
	auto pit(useridx.equal_range(user.get_acct()));
	for(; pit.first != pit.second; ++pit.first)
	{
		const auto &id(pit.first->second);
		closure(get(id));
	}
}


void Voting::for_each(const Chan &chan,
                      const std::function<void (const Vote &vote)> &closure)
const
{
	auto pit(chanidx.equal_range(chan.get_name()));
	for(; pit.first != pit.second; ++pit.first)
	{
		const auto &id(pit.first->second);
		closure(get(id));
	}
}


void Voting::for_each(const Chan &chan,
                      const User &user,
                      const std::function<void (const Vote &vote)> &closure)
const
{
	const auto ids(get_ids(chan,user));
	for(const auto &id : ids)
		closure(get(id));
}


Vote &Voting::get(const id_t &id)
try
{
	return *votes.at(id);
}
catch(const std::out_of_range &e)
{
	throw Exception("There is no active vote with that ID.");
}


const Vote &Voting::get(const id_t &id)
const try
{
	return *votes.at(id);
}
catch(const std::out_of_range &e)
{
	throw Exception("There is no active vote with that ID.");
}


const id_t &Voting::get_id(const User &user)
const
{
	if(count(user) > 1)
		throw Exception("There are multiple votes initiated by this user. Specify an ID#.");

	const auto it(useridx.find(user.get_acct()));
	if(it == useridx.end())
		throw Exception("There are no active votes initiated by this user.");

	return it->second;
}


const id_t &Voting::get_id(const Chan &chan)
const
{
	if(count(chan) > 1)
		throw Exception("There are multiple votes active in this channel. Specify an ID#.");

	const auto it(chanidx.find(chan.get_name()));
	if(it == chanidx.end())
		throw Exception("There are no active votes in this channel.");

	return it->second;
}


std::vector<id_t> Voting::get_ids(const Chan &chan,
                                  const User &user)
const
{
	const auto cpit(chanidx.equal_range(chan.get_name()));
	if(cpit.first == cpit.second)
		return {};

	const auto upit(useridx.equal_range(user.get_acct()));
	if(upit.first == upit.second)
		return {};

	static const auto second([](const auto &p) { return p.second; });
	std::vector<id_t> cids(std::distance(cpit.first,cpit.second));
	std::vector<id_t> uids(std::distance(upit.first,upit.second));
	std::transform(cpit.first,cpit.second,cids.begin(),second);
	std::transform(upit.first,upit.second,uids.begin(),second);
	std::sort(cids.begin(),cids.end());
	std::sort(uids.begin(),uids.end());

	std::vector<id_t> ret(std::min(cids.size(),uids.size()));
	std::set_intersection(cids.begin(),cids.end(),uids.begin(),uids.end(),ret.begin());
	return ret;
}
