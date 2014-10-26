/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include "bot.h"

using namespace irc::bot;


static std::mutex mutex;
static std::condition_variable cond;
static std::atomic<bool> interrupted;
static std::deque<std::pair<Bot *, Msg>> queue;

static void worker();
static std::thread thread {&worker};
static scope join([]
{
	interrupted.store(true,std::memory_order_release);
	cond.notify_all();
	thread.join();
});


static
decltype(queue)::value_type next()
{
	std::unique_lock<std::mutex> lock(mutex);
	cond.wait(lock,[]
	{
		if(interrupted.load(std::memory_order_consume))
			throw Internal("Interrupted");

		return !queue.empty();
	});

	auto ret = std::move(queue.front());
	queue.pop_front();
	return ret;
}


static
void worker()
try
{
	while(1)
	{
		auto msgp = next();
		Bot &bot = *std::get<0>(msgp);
		const Msg &msg = std::get<1>(msgp);
		const std::lock_guard<Bot> lock(bot);
		bot.dispatch(msg);
	}
}
catch(const Internal &e)
{
	return;
}


template<class event_t>
void handler(irc_session_t *const &session,
             event_t&& event,
             const char *const &origin,
             const char **params,
             unsigned int &count)
try
{
	void *const ctx = irc_get_ctx(session);

	if(!ctx)
	{
		fprintf(stderr, "irc_get_ctx(session = %p) NULL CONTEXT!\n",ctx);
		return;
	}

	Bot *const &bot = static_cast<Bot *>(ctx);
	const std::lock_guard<std::mutex> lock(mutex);
	queue.emplace_back(bot,Msg{event,origin,params,count});
	cond.notify_one();
	coarse_flood_done();
}
catch(const std::exception &e)
{
	std::cerr
	<< "\033[1;37;41mException:\033[0m "
	<< "[\033[1;31m" << e.what() << "\033[0m] "
	<< "handler"
	<< "(" << session
	<< "," << event
	<< "," << origin
	<< "," << params
	<< "," << count
	<< ")"
	<< "\033[0m"
	<< std::endl;
}


static
void handler_numeric(irc_session_t *const session,
                     unsigned int event,
                     const char *const origin,
                     const char **params,
                     unsigned int count)
{
/*
	printf("DEBUG: (session: %p event: %u origin: %s params: %p count: %u)\n",
	       session,
	       event,
	       origin,
	       params,
	       count);
*/
	handler(session,event,origin,params,count);
}


static
void handler_named(irc_session_t *const session,
                   const char *const event,
                   const char *const origin,
                   const char **params,
                   unsigned int count)
{
/*
	printf("DEBUG: (session: %p event: %s origin: %s params: %p count: %u)\n",
	       session,
	       event,
	       origin,
	       params,
	       count);
*/
	handler(session,event,origin,params,count);
}


// Event handler globals for libircclient (callbacks.h)
irc::bot::Callbacks irc::bot::callbacks {&handler_numeric,&handler_named};
