/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include "bot.h"

using namespace irc::bot;


decltype(SendQ::mutex) SendQ::mutex;
decltype(SendQ::interrupted) SendQ::interrupted;
decltype(SendQ::cond) SendQ::cond;
decltype(SendQ::queue) SendQ::queue;

static const uint32_t CALL_MAX_ATTEMPTS = 25;
static constexpr std::chrono::milliseconds CALL_THROTTLE {750};

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
	for(size_t i = 0; i < CALL_MAX_ATTEMPTS; i++) try
	{
		irc_call(sess,irc_send_raw,"%s",pck.c_str());
		return;
	}
	catch(const Exception &e)
	{
		// No more reattempts at the limit
		if(i >= CALL_MAX_ATTEMPTS - 1)
			throw;

		switch(e.code())
		{
			case LIBIRC_ERR_NOMEM:
				// Sent too much data without letting libirc recv(), we can throttle and try again.
				std::cerr << "call(): \033[1;33mthrottling send\033[0m"
				          << " (attempt: " << i << " timeout: " << CALL_THROTTLE.count() << ")"
				          << std::endl;

				std::this_thread::sleep_for(CALL_THROTTLE);
				continue;

			default:
				// No reattempt for everything else
				throw;
		}
	}
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
