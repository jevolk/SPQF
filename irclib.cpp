/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <unordered_map>
#include <functional>
#include <iomanip>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <ostream>
#include <atomic>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <libircclient.h>
#include <libirc_rfcnumeric.h>

#include "util.h"
#include "mode.h"
#include "mask.h"
#include "ban.h"
#include "msg.h"
#include "sess.h"
#include "user.h"
#include "chan.h"
#include "users.h"
#include "chans.h"
#include "bot.h"
#include "irclib.h"


Callbacks bot_cbs;        // Event handler globals for libircclient (irclib.h)


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

	Bot &bot = *static_cast<Bot *>(ctx);
	bot(event,origin,params,count);
}
catch(const std::exception &e)
{
	std::cerr
	<< "\033[1;31m"
	<< "Exception: [" << e.what() << "] "
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


Callbacks::Callbacks(void)
{
	event_numeric         = &handler_numeric;
	event_unknown         = &handler_named;
	event_connect         = &handler_named;
	event_nick            = &handler_named;
	event_quit            = &handler_named;
	event_join            = &handler_named;
	event_part            = &handler_named;
	event_mode            = &handler_named;
	event_umode           = &handler_named;
	event_topic           = &handler_named;
	event_kick            = &handler_named;
	event_channel         = &handler_named;
	event_privmsg         = &handler_named;
	event_notice          = &handler_named;
	event_channel_notice  = &handler_named;
	event_invite          = &handler_named;
	event_ctcp_req        = &handler_named;
	event_ctcp_rep        = &handler_named;
	event_ctcp_action     = &handler_named;
	//event_dcc_chat_req    = &handler_dcc;
	//event_dcc_send_req    = &handler_dcc;
}
