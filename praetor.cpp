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


Praetor::Praetor(Sess &sess,
                 Chans &chans,
                 Users &users,
                 Bot &bot,
                 Vdb &vdb,
                 Logs &logs):
sess(sess),
chans(chans),
users(users),
bot(bot),
vdb(vdb),
logs(logs),
interrupted(false),
thread(&Praetor::worker,this),
init_thread(&Praetor::init,this)
{
}


Praetor::~Praetor()
noexcept
{
	interrupted.store(true,std::memory_order_release);
	cond.notify_all();
	init_thread.join();
	thread.join();
}


void Praetor::init()
{
	const std::lock_guard<Bot> l(bot);
	std::cout << "[Praetor]: Initiating the schedule."
	          << " Reading " << vdb.count() << " votes..."
	          << std::endl;

	auto it(vdb.cbegin(stldb::SNAPSHOT));
	const auto end(vdb.cend());
	for(; it != end && !interrupted.load(std::memory_order_consume); ++it) try
	{
		add(Adoc(it->second));
	}
	catch(const Exception &e)
	{
		const auto id(lex_cast<id_t>(it->first));
		std::cerr << "[Praetor]: Init reading #" << id << ": \033[1;31m" << e << "\033[0m" << std::endl;
	}
}


void Praetor::worker()
{
	while(!interrupted.load(std::memory_order_consume)) try
	{
		process();
	}
	catch(const Internal &e)
	{
		std::cerr << "[Praetor]: \033[1;41m" << e << "\033[0m" << std::endl;
	}
}


void Praetor::process()
{
	using system_clock = std::chrono::system_clock;

	std::unique_lock<decltype(mutex)> lock(mutex);
	cond.wait_until(lock,system_clock::from_time_t(sched.next_abs()));

	while(sched.next_rel() <= 0)
	{
		const id_t id(sched.next_id());
		sched.pop();

		const unlock_guard<decltype(lock)> unlock(lock);
		if(!process(id))
		{
			const time_t retry_absolute(time(NULL) + 300);
			add(id,retry_absolute);
		}
	}
}


bool Praetor::process(const id_t &id)
{
	const std::unique_lock<Bot> lock(bot);
	const std::unique_ptr<Vote> vote(vdb.get(id,&sess,&chans,&users,&logs));
	return process(*vote);
}


bool Praetor::process(Vote &vote)
noexcept try
{
	if(!chans.has(vote.get_chan_name()))
		return false;

	vote.expire();
	return true;
}
catch(const std::exception &e)
{
	std::cerr << "\033[1;31m"
	          << "[Praetor]:"
	          << " Vote #" << vote.get_id()
	          << " error: " << e.what()
	          << "\033[0m"
	          << std::endl;

	return false;
}


void Praetor::add(const Adoc &doc)
{
	const id_t id = lex_cast<id_t>(doc["id"]);
	const time_t ended = secs_cast(doc["ended"]);
	const time_t expiry = secs_cast(doc["expiry"]);
	const time_t cfgfor = secs_cast(doc["cfg.for"]);
	if(expiry || !cfgfor || !ended || doc.has("reason"))
		return;

	const time_t absolute = ended + cfgfor;
	printf("%lu [Praetor]: Scheduling #%u [expiry: %ld cfgfor: %ld ended: %ld] absolute: %ld relative: %ld\n",
	       time(NULL),
	       id,
	       expiry,
	       cfgfor,
	       ended,
	       absolute,
	       absolute - time(NULL));

	add(id,absolute);
}


void Praetor::add(std::unique_ptr<Vote> &&vote)
{
	const id_t id = vote->get_id();
	const auto cfg = vote->get_cfg();
	const time_t absolute = time(NULL) + secs_cast(cfg["for"]);
	add(id,absolute);
}


void Praetor::add(const id_t &id,
                  const time_t &absolute)
{
	const std::lock_guard<std::mutex> l(mutex);
	sched.add(id,absolute);
	cond.notify_one();
}



Schedule::id_t Schedule::pop()
{
	const auto ret = std::get<id_t>(sched.at(0));
	sched.pop_front();
	return ret;
}


void Schedule::add(const id_t &id,
                   const time_t &absolute)
{
	sched.emplace_back(std::make_tuple(id,absolute));
	sort();
}


void Schedule::sort()
{
	std::sort(sched.begin(),sched.end(),[]
	(const auto &a, const auto &b)
	{
		return std::get<time_t>(a) < std::get<time_t>(b);
	});
}
