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
                 Vdb &vdb):
sess(sess),
chans(chans),
users(users),
bot(bot),
vdb(vdb),
interrupted(false),
thread(&Praetor::worker,this)
{
	std::thread(&Praetor::init,this).detach();

}


Praetor::~Praetor()
noexcept
{
	interrupted.store(true,std::memory_order_release);
	cond.notify_all();
	thread.join();
}


void Praetor::init()
{
	std::this_thread::sleep_for(std::chrono::seconds(30));
	std::cout << "[Praetor]: Initiating the schedule."
	          << " Reading " << vdb.count() << " votes..."
	          << std::endl;

	auto it = vdb.cbegin(stldb::SNAPSHOT);
	auto end = vdb.cend();
	for(; it != end; ++it)
	{
		if(interrupted.load(std::memory_order_consume))
			break;

		const auto id = lex_cast<id_t>(it->first);
		const Adoc doc(it->second);

		const time_t ended = secs_cast(doc["ended"]);
		const time_t expiry = secs_cast(doc["expiry"]);
		const time_t cfgfor = secs_cast(doc["cfg.for"]);
		if(expiry || !cfgfor || !ended)
			continue;

		const time_t absolute = ended + cfgfor;
		printf("[Praetor]: Scheduling #%u [expiry: %ld cfgfor: %ld ended: %ld] absolute: %ld relative: %ld\n",
		       id,
		       expiry,
		       cfgfor,
		       ended,
		       absolute,
		       absolute - time(NULL));

		add(id,absolute);
	}
}


void Praetor::worker()
{
	while(!interrupted.load(std::memory_order_consume))
		process();
}


void Praetor::process()
{
	std::unique_lock<decltype(mutex)> lock(mutex);
	cond.wait_for(lock,std::chrono::seconds(sched.next_rel()));

	while(sched.next_rel() <= 0)
	{
		const id_t id = sched.next_id();
		sched.pop();

		const unlock_guard<decltype(lock)> unlock(lock);
		process(id);
	}
}


void Praetor::process(const id_t &id)
{
	const std::unique_lock<Bot> lock(bot);
	const std::unique_ptr<Vote> vote = vdb.get(id,&sess,&chans,&users);
	process(*vote);
}


void Praetor::process(Vote &vote)
noexcept try
{
	vote.expire();
}
catch(const std::exception &e)
{
	std::cerr << "\033[1;31m"
	          << "[Praetor]:"
	          << " Vote #" << vote.get_id()
	          << " error: " << e.what()
	          << "\033[0m"
	          << std::endl;
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
