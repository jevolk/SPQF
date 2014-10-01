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

#include <leveldb/filter_policy.h>
#include <leveldb/cache.h>
#include <leveldb/db.h>

#include "util.h"
#include "mode.h"
#include "mask.h"
#include "ban.h"
#include "msg.h"
#include "sess.h"
#include "ldb.h"
#include "locutor.h"
#include "user.h"
#include "chan.h"
#include "users.h"
#include "chans.h"
#include "bot.h"
#include "respub.h"
#include "irclib.h"


// Pointer to instance for signals
static Bot *bot;

static
void handle_sig(const int sig)
{
	switch(sig)
	{
		case SIGINT:   printf("INTERRUPT...\n");  break;
		case SIGTERM:  printf("TERMINATE...\n");  break;
	}

	if(bot)
		bot->quit();
}


int main(int argc, char **argv)
try
{
	if(argc < 4)
	{
		printf("usage: ./%s <host> <port> [nick] [chan]...\n",argv[0]);
		return -1;
	}

	struct Ident id;
	id.host = argv[1];
	id.port = atoi(argv[2]);
	id.nick = argc > 3? argv[3] : "ResPublica";
	for(int i = 4; i < argc; i++)
		id.autojoin.emplace_back(argv[i]);

	Callbacks &cbs = bot_cbs;                  // irclib.cpp extern callbacks structure
	ResPublica bot(id,cbs);                    // Create instance of the bot
	::bot = &bot;                              // Set pointer for sighandlers
	signal(SIGINT,&handle_sig);                // Register handler for ctrl-c
	signal(SIGTERM,&handle_sig);               // Register handler for term
	bot.conn();                                // Connect to server (may throw)
	bot.run();                                 // Loops in foreground forever
}
catch(const Exception &e)
{
	std::cerr << "Exception: " << e << std::endl;
	return -1;
}
