/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include <signal.h>

// libircbot irc::bot::
#include "ircbot/bot.h"

// SPQF
using namespace irc::bot;
#include "vote.h"
#include "votes.h"
#include "vdb.h"
#include "praetor.h"
#include "voting.h"
#include "respub.h"


// Pointer to instance for signals
static Bot *bot;


static
void handle_sig(const int sig)
{
	switch(sig)
	{
		case SIGINT:    std::cout << "INTERRUPT..." << std::endl;   break;
		case SIGTERM:   std::cout << "TERMINATE..." << std::endl;   break;
	}

	if(bot)
	{
		const std::lock_guard<Bot> lock(*bot);
		bot->quit();
	}
}


int main(int argc, char **argv) try
{
	Opts opts;
	opts.parse({argv+1,argv+argc});

	if(opts["nick"].empty())
	{
		fprintf(stderr,"usage: %s [ --var=val | --join=chan ] [...]\n",argv[0]);
		fprintf(stderr,"\ndefaults:\n");
		for(const auto &kv : opts)
			fprintf(stderr,"\t--%s=\"%s\"\n",kv.first.c_str(),kv.second.c_str());

		return -1;
	}

	printf("****** \033[1mSENATVS POPVLVS QVE FREENODUS\033[0m ******\n");
	printf("** The Senate and The People of Freenode **\n\n");
	printf("Current configuration:\n");
	std::cout << opts << std::endl;

	srand(getpid());
	ResPublica bot(opts);                 // Create instance of the bot
	::bot = &bot;                         // Set pointer for sighandlers
	signal(SIGINT,&handle_sig);           // Register handler for ctrl-c
	signal(SIGTERM,&handle_sig);          // Register handler for term
	bot.connect();                        // Connect to server (may throw)
	bot(bot.FOREGROUND);                  // Loops in foreground forever
}
catch(const std::exception &e)
{
	std::cerr << "Exception: " << e.what() << std::endl;
	return -1;
}
