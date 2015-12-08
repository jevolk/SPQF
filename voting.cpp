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
#include "vdb.h"
#include "praetor.h"
#include "voting.h"


Voting::Voting(Sess &sess,
               Chans &chans,
               Users &users,
               Logs &logs,
               Bot &bot,
               Vdb &vdb,
               Praetor &praetor):
sess(sess),
chans(chans),
users(users),
logs(logs),
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
	Vote &vote = get(id);
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

	vote.cancel();
	del(vote.get_id());
}


void Voting::eligible_worker()
{
	{
		std::unique_lock<decltype(mutex)> lock(mutex);
		sem.wait(lock,[this]
		{
			return initialized.load(std::memory_order_consume) ||
			       interrupted.load(std::memory_order_consume);
		});
	}

	while(!interrupted.load(std::memory_order_consume)) try
	{
		eligible_add();
		eligible_sleep();
	}
	catch(const Internal &e)
	{
		std::cerr << "[Voting (eligible worker)]: \033[1;41m" << e << "\033[0m" << std::endl;
	}
}


void Voting::eligible_add()
{
	const std::lock_guard<Bot> lock(bot);
	chans.for_each([this](Chan &chan)
	{
		eligible_add(chan);
	});
}


bool Voting::eligible_sleep()
{
	using std::chrono::steady_clock;

	const auto now(steady_clock::now());
	const auto fut(now + std::chrono::hours(24));
	std::unique_lock<decltype(mutex)> lock(mutex);
	return sem.wait_until(lock,fut) == std::cv_status::timeout;
}


void Voting::eligible_add(Chan &chan)
try
{
	const auto civis(chan.get("config.vote.civis"));
	const auto age(secs_cast(civis["eligible.age"]));
	const auto lines(civis.get<uint>("eligible.lines",0));
	const Adoc excludes(civis.get_child("eligible.exclude",Adoc()));
	const auto exclude(excludes.into<std::set<std::string>>());
	if(lines == 0 || age == 0)
		return;

	std::cout << "Finding eligible for channel " << chan.get_name() << std::endl;

	Logs::SimpleFilter filt;
	filt.type = "PRI";  // PRIVMSG
	filt.time.first = 0;
	filt.time.second = time(NULL) - age;
	std::map<std::string,uint> count;
	std::map<std::string,std::string> accts;
	logs.for_each(chan.get_name(),filt,[&count,&accts]
	(const Logs::ClosureArgs &a)
	{
		if(strlen(a.acct) == 0 || *a.acct == '*')
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

		const auto &nick(accts.at(acct));
		if(!users.has(nick))
			continue;

		User &user(users.get(nick));
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

		const auto qid(eligible_last_vote(chan,nick,"quiet"));
		const auto bid(eligible_last_vote(chan,nick,"ban"));
		const auto &id(std::max(qid,bid));
		if(id)
		{
			filt.acct = acct;
			filt.time.first = lex_cast<time_t>(vdb.get_value(id,"ended"));
			const auto recount(logs.count(chan.get_name(),filt));
			if(recount < lines)
				continue;
		}

		User &myself(users.get(sess.get_nick()));
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


Voting::id_t Voting::eligible_last_vote(const Chan &chan,
                                        const std::string &issue,
                                        const std::string &type)
{
	const std::forward_list<Vdb::Term> terms
	{
		{ "type",    type             },
		{ "issue",   tolower(issue)   },
		{ "reason",  ""               },
		{ "chan",    chan.get_name()  },
	};

	auto res(vdb.find(terms));
	if(res.empty())
		return 0;    // note sentinel 0 is a valid vote id but ignored here

	res.reverse();
	res.resize(1);
	return res.front();
}


void Voting::remind_worker()
{
	{
		std::unique_lock<decltype(mutex)> lock(mutex);
		sem.wait(lock,[this]
		{
			return initialized.load(std::memory_order_consume) ||
			       interrupted.load(std::memory_order_consume);
		});
	}

	while(!interrupted.load(std::memory_order_consume)) try
	{
		remind_votes();
		remind_sleep();
	}
	catch(const Internal &e)
	{
		std::cerr << "[Voting (remind worker)]: \033[1;41m" << e << "\033[0m" << std::endl;
	}
}


bool Voting::remind_sleep()
{
	using std::chrono::steady_clock;

	const auto now(steady_clock::now());
	const auto fut(now + std::chrono::hours(24));
	std::unique_lock<decltype(mutex)> lock(mutex);
	return sem.wait_until(lock,fut) == std::cv_status::timeout;
}


void Voting::remind_votes()
{
	using namespace colors;

	const std::unique_lock<Bot> lock(bot);
	static const milliseconds delay(750);
	//const FloodGuard guard(bot.sess,delay);

	for(auto it(votes.cbegin()); it != votes.cend(); ++it)
	{
		const Vote &vote(*it->second);
		if(!chans.has(vote.get_chan_name()))
			continue;

		const auto &cfg(vote.get_cfg());
		if(!cfg.get<bool>("remind.enable",false))
			continue;

		Chan &chan(vote.get_chan());
		chan.users.for_each([&](User &user)
		{
			if(vote.voted(user))
				return;

			if(!vote.enfranchised(user))
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
		poll_sleep();
	}
	catch(const Internal &e)
	{
		std::cerr << "[Voting (poll worker)]: \033[1;41m" << e << "\033[0m" << std::endl;
	}
}


void Voting::poll_init()
{
	const std::lock_guard<Bot> lock(bot);
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
			auto vote(vdb.get(id,&sess,&chans,&users,&logs));
			const auto iit(votes.emplace(id,std::move(vote)));
			{
				const auto &vote(*iit.first->second);
				chanidx.emplace(vote.get_chan_name(),id);
				useridx.emplace(vote.get_user_acct(),id);
			}
		}
	}
	catch(const Exception &e)
	{
		const auto id(lex_cast<id_t>(it->first));
		std::cerr << "[Voting]: Failed reading #" << id << ": \033[1;31m" << e << "\033[0m" << std::endl;
	}
}


void Voting::poll_sleep()
{
	//TODO: calculate next deadline. Recalculate on semaphore.
	std::unique_lock<decltype(mutex)> lock(mutex);
	sem.wait_for(lock,std::chrono::seconds(2));
}


void Voting::poll_votes()
{
	const std::unique_lock<Bot> lock(bot);
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
	const Vote::id_t &id = it->first;
	auto vote = std::move(it->second);

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


Vote::id_t Voting::get_next_id()
const
{
	id_t i = 0;
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
	auto pit = useridx.equal_range(user.get_acct());
	for(; pit.first != pit.second; ++pit.first)
	{
		const auto &id = pit.first->second;
		closure(get(id));
	}
}


void Voting::for_each(const Chan &chan,
                      const std::function<void (Vote &vote)> &closure)
{
	auto pit = chanidx.equal_range(chan.get_name());
	for(; pit.first != pit.second; ++pit.first)
	{
		const auto &id = pit.first->second;
		closure(get(id));
	}
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
	auto pit = useridx.equal_range(user.get_acct());
	for(; pit.first != pit.second; ++pit.first)
	{
		const auto &id = pit.first->second;
		closure(get(id));
	}
}


void Voting::for_each(const Chan &chan,
                      const std::function<void (const Vote &vote)> &closure)
const
{
	auto pit = chanidx.equal_range(chan.get_name());
	for(; pit.first != pit.second; ++pit.first)
	{
		const auto &id = pit.first->second;
		closure(get(id));
	}
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


const Voting::id_t &Voting::get_id(const User &user)
const
{
	if(count(user) > 1)
		throw Exception("There are multiple votes initiated by this user. Specify an ID#.");

	const auto it = useridx.find(user.get_acct());
	if(it == useridx.end())
		throw Exception("There are no active votes initiated by this user.");

	return it->second;
}


const Voting::id_t &Voting::get_id(const Chan &chan)
const
{
	if(count(chan) > 1)
		throw Exception("There are multiple votes active in this channel. Specify an ID#.");

	const auto it = chanidx.find(chan.get_name());
	if(it == chanidx.end())
		throw Exception("There are no active votes in this channel.");

	return it->second;
}
