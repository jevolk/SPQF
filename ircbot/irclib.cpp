/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include "bot.h"


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

	irc::bot::Bot &bot = *static_cast<irc::bot::Bot *>(ctx);
	bot(event,origin,params,count);
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
