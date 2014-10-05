/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


// libstd
#include <signal.h>
#include <forward_list>
#include <condition_variable>
#include <thread>

// libircbot irc::bot::
#include "ircbot/bot.h"

// SPQF
using namespace irc::bot;
#include "vote.h"
#include "votes.h"
#include "voting.h"
#include "respub.h"


// Pointer to instance for signals
static irc::bot::Bot *instance;

static
void handle_sig(const int sig)
{
	switch(sig)
	{
		case SIGINT:   printf("INTERRUPT...\n");  break;
		case SIGTERM:  printf("TERMINATE...\n");  break;
	}

	if(instance)
		instance->quit();
}


int main(int argc, char **argv)
try
{
	if(argc < 2)
	{
		printf("usage: %s <host> [port] [nick] [chan]...\n",argv[0]);
		return -1;
	}

	struct irc::bot::Ident id;
	id["hostname"] = argv[1];
	id["port"] = argc > 2? argv[2] : "6667";
	id["nickname"] = argc > 3? argv[3] : "ResPublica";
	for(int i = 4; i < argc; i++)
		id.autojoin.emplace_back(argv[i]);

	irc::bot::Callbacks &cbs = bot_cbs;        // irclib.cpp extern callbacks structure
	ResPublica instance(id,cbs);               // Create instance of the bot
	::instance = &instance;                    // Set pointer for sighandlers
	signal(SIGINT,&handle_sig);                // Register handler for ctrl-c
	signal(SIGTERM,&handle_sig);               // Register handler for term
	instance.conn();                           // Connect to server (may throw)
	instance.run();                            // Loops in foreground forever
}
catch(const irc::bot::Exception &e)
{
	std::cerr << "Exception: " << e << std::endl;
	return -1;
}
