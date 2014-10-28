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


}


Praetor::~Praetor()
noexcept
{
	interrupted.store(true,std::memory_order_release);
	cond.notify_all();
	thread.join();
}


void Praetor::add(const id_t &id)
{


}


void Praetor::add(std::unique_ptr<Vote> &&vote)
{
	const id_t id = vote->get_id();
	const auto cfg = vote->get_cfg();
	const time_t absolute = time(NULL) + cfg.get<time_t>("for");
	const std::lock_guard<std::mutex> l(mutex);
	sched.add(id,absolute);
	cond.notify_one();
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
	const auto type = vdb.get_type(id);
	std::cout << type << std::endl;

	const std::unique_lock<Bot> lock(bot);
	const std::unique_ptr<Vote> vote = vdb.get(id,&sess,&chans,&users);
	process(*vote);
}


void Praetor::process(Vote &vote)
noexcept try
{
	vote.revert();
}
catch(const std::exception &e)
{
	std::cerr << "Praetor::Process():"
	          << " Vote #" << vote.get_id()
	          << " error: " << e.what()
	          << std::endl;
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
