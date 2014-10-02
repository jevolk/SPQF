/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include <signal.h>
#include <forward_list>
#include <thread>

#include "ircbot/bot.h"
#include "respub.h"


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
	id["hostname"] = argv[1];
	id["port"] = argv[2];
	id["nickname"] = argc > 3? argv[3] : "ResPublica";
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
