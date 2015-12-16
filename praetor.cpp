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


Praetor::Praetor(Bot &bot,
                 Vdb &vdb):
bot(bot),
vdb(vdb),
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
	bot.set_tls_context();
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
		const auto &id(it->first);
		std::cerr << "[Praetor]: Init reading #" << id
		          << ": \033[1;31m" << e << "\033[0m"
		          << std::endl;
	}
}


void Praetor::worker()
{
	{
		const std::lock_guard<Bot> l(bot);
		bot.set_tls_context();
	}

	while(!interrupted.load(std::memory_order_consume)) try
	{
		process();
	}
	catch(const Internal &e)
	{
		std::cerr << "[Praetor]: \033[1;41m" << e << "\033[0m"
		          << std::endl;
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
	const std::unique_ptr<Vote> vote(vdb.get(id));
	return process(*vote);
}


bool Praetor::process(Vote &vote)
noexcept try
{
	const auto &chans(get_chans());
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
try
{
	const auto id(lex_cast<id_t>(doc["id"]));
	const auto ended(secs_cast(doc["ended"]));
	const auto expiry(secs_cast(doc["expiry"]));
	const auto cfgfor(secs_cast(doc["cfg.for"]));
	if(expiry || (cfgfor <= 0) || !ended || doc.has("reason"))
		return;

	const time_t absolute(ended + cfgfor);
	printf("%lu [Praetor]: Scheduling #%u [type: %s chan: %s cfgfor: %ld ended: %ld] absolute: %ld relative: %ld\n",
	       time(NULL),
	       id,
	       doc["type"].c_str(),
	       doc["chan"].c_str(),
	       cfgfor,
	       ended,
	       absolute,
	       absolute - time(NULL));

	add(id,absolute);
}
catch(const std::exception &e)
{
	std::cerr << "\033[1;31m"
	          << "[Praetor]: add(doc): "
	          << " Vote #" << doc["id"]
	          << " error: " << e.what()
	          << "\033[0m"
	          << std::endl;
}


void Praetor::add(std::unique_ptr<Vote> &&vote)
try
{
	const id_t &id(vote->get_id());
	const auto &cfg(vote->get_cfg());
	const auto cfgfor(secs_cast(cfg["for"]));
	if(cfgfor <= 0)
		return;

	const time_t absolute(time(NULL) + cfgfor);
	add(id,absolute);
}
catch(const std::exception &e)
{
	std::cerr << "\033[1;31m"
	          << "[Praetor]: add(vote): "
	          << " Vote #" << vote->get_id()
	          << " error: " << e.what()
	          << "\033[0m"
	          << std::endl;
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
