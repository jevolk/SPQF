/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include "bot.h"

using namespace irc::bot;


decltype(SendQ::mutex) SendQ::mutex, SendQ::flood_mutex;
decltype(SendQ::cond) SendQ::cond, SendQ::flood_cond;
decltype(SendQ::interrupted) SendQ::interrupted;
decltype(SendQ::queue) SendQ::queue;

static std::thread thread {&SendQ::worker};
static scope join([]
{
	SendQ::interrupt();
	thread.join();
});


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
			case LIBIRC_ERR_NOMEM:
				SendQ::flood_wait();
				continue;

			default:
				// No reattempt for everything else
				throw;
		}
	}
}


void SendQ::flood_wait()
{
	std::unique_lock<decltype(flood_mutex)> lock(flood_mutex);
	flood_cond.wait(lock);
}


void SendQ::flood_done()
{
	flood_cond.notify_all();
}


void SendQ::interrupt()
{
	interrupted.store(true,std::memory_order_release);
	cond.notify_all();
}


void SendQ::worker()
try
{
	while(1)
	{
		std::unique_lock<decltype(SendQ::mutex)> lock(SendQ::mutex);
		cond.wait(lock,[&]
		{
			if(interrupted.load(std::memory_order_consume))
				throw Internal("Interrupted");

			return !queue.empty();
		});

		auto &ent = queue.front();
		const scope pf([]{ queue.pop_front(); });
		send(ent.sess,ent.pck);
    }
}
catch(const Internal &e)
{
	return;
}
