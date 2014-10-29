/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include "bot.h"

using namespace irc::bot;



///////////////////////////////////////////////////////////////////////////////
//
// libircclient
//

decltype(coarse_flood_mutex) irc::bot::coarse_flood_mutex;
decltype(coarse_flood_cond) irc::bot::coarse_flood_cond;


void irc::bot::coarse_flood_wait()
{
	std::unique_lock<std::mutex> lock(coarse_flood_mutex);
	coarse_flood_cond.wait(lock);
}


void irc::bot::coarse_flood_done()
{
	coarse_flood_cond.notify_all();
}


static
void send(irc_session_t *const &sess,
          const std::string &pck)
{
	while(1) try
	{
		irc_call(sess,irc_send_raw,"%s",pck.c_str());
		return;
	}
	catch(const Internal &e)
	{
		switch(e.code())
		{
			case LIBIRC_ERR_NOMEM:   coarse_flood_wait();   continue;
			default:                                        throw;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
//
// SendQ
//

decltype(SendQ::mutex) SendQ::mutex;
decltype(SendQ::cond) SendQ::cond;
decltype(SendQ::interrupted) SendQ::interrupted;
decltype(SendQ::queue) SendQ::queue;
decltype(SendQ::slowq) SendQ::slowq;

static std::thread thread {&SendQ::worker};
static scope join([]
{
	SendQ::interrupt();
	thread.join();
});


void SendQ::interrupt()
{
	interrupted.store(true,std::memory_order_release);
	cond.notify_all();
}


auto SendQ::slowq_next()
{
	using namespace std::chrono;

	if(slowq.empty())
		return milliseconds(std::numeric_limits<uint32_t>::max());

	const auto now = steady_clock::now();
	const auto &abs = slowq.front().absolute;
	if(abs < now)
		return milliseconds(0);

	return duration_cast<milliseconds>(abs.time_since_epoch()) -
	       duration_cast<milliseconds>(now.time_since_epoch());
}


void SendQ::slowq_process()
{
	while(!slowq.empty())
	{
		const Ent &ent = slowq.front();
		if(ent.absolute > steady_clock::now())
			break;

		send(ent.sess,ent.pck);
		slowq.pop_front();
	}
}


void SendQ::slowq_add(Ent &ent)
{
	slowq.emplace_back(ent);
	std::sort(slowq.begin(),slowq.end(),[]
	(const Ent &a, const Ent &b)
	{
		return a.absolute < b.absolute;
	});
}


void SendQ::process(Ent &ent)
{
	if(ent.absolute > steady_clock::now())
		slowq_add(ent);
	else
		send(ent.sess,ent.pck);
}


void SendQ::worker()
try
{
	while(1)
	{
		std::unique_lock<decltype(SendQ::mutex)> lock(SendQ::mutex);
		cond.wait_for(lock,slowq_next());

		if(interrupted.load(std::memory_order_consume))
			throw Internal(-1,"Interrupted");

		while(!queue.empty())
		{
			const scope pf([]{ queue.pop_front(); });
			process(queue.front());
		}

		slowq_process();
    }
}
catch(const Internal &e)
{
	switch(e.code())
	{
		case -1:                                                  return;
		default:   std::cerr << "[sendq]: " << e << std::endl;    throw;
	}
}
