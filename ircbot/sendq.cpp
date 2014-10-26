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


using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

struct Ent
{
	irc_session_t *sess;
	std::string pck;
	TimePoint time;
};

struct State
{
	TimePoint last;

};

static const std::chrono::milliseconds PROCESS_INTERVAL {250};
static std::map<irc_session_t *, State> state;
static std::deque<Ent> slowq;


static
void process_slowq()
{
	//TODO:                                               //
	std::this_thread::sleep_for(std::chrono::milliseconds(333));

}


static
void process(irc_session_t *const &sess,
             const std::string &pck)
{
	state[sess].last = std::chrono::steady_clock::now();
	send(sess,pck);
}


SendQ::Ent SendQ::next()
{
	std::unique_lock<decltype(SendQ::mutex)> lock(SendQ::mutex);
	cond.wait_for(lock,PROCESS_INTERVAL,[&]
	{
		if(interrupted.load(std::memory_order_consume))
			throw Internal("Interrupted");

		return !queue.empty();
	});

	if(queue.empty())
		return {nullptr};

	const scope pf([]{ queue.pop_front(); });
	return queue.front();
}


void SendQ::worker()
try
{
	while(1)
	{
		const Ent ent = next();
		if(ent.sess)
			process(ent.sess,ent.pck);

		process_slowq();
    }
}
catch(const Internal &e)
{
	return;
}
