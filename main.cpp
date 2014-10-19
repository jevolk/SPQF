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
#include "vdb.h"
#include "vote.h"
#include "votes.h"
#include "voting.h"
#include "respub.h"


// Pointer to instance for signals
static Bot *instance;


static
void handle_sig(const int sig)
{
	switch(sig)
	{
		case SIGINT:    std::cout << "INTERRUPT..." << std::endl;   break;
		case SIGTERM:   std::cout << "TERMINATE..." << std::endl;   break;
	}

	if(instance)
	{
		const std::lock_guard<std::mutex> lock(*instance);
		instance->quit();
	}
}


int main(int argc, char **argv) try
{
	Ident id;
	id.parse({argv+1,argv+argc});

	if(id["host"].empty())
	{
		fprintf(stderr,"usage: %s [ --var=val | --join=chan ] [...]\n",argv[0]);
		fprintf(stderr,"\ndefaults:\n");
		for(const auto &kv : id)
			fprintf(stderr,"\t--%s=\"%s\"\n",kv.first.c_str(),kv.second.c_str());

		return -1;
	}

	printf("****** \033[1mSENATVS POPVLVS QVE FREENODUS\033[0m ******\n");
	printf("** The Senate and The People of Freenode **\n\n");
	std::cout << id << std::endl;

	srand(getpid());
	ResPublica instance(id);                   // Create instance of the bot
	::instance = &instance;                    // Set pointer for sighandlers
	signal(SIGINT,&handle_sig);                // Register handler for ctrl-c
	signal(SIGTERM,&handle_sig);               // Register handler for term
	instance.conn();                           // Connect to server (may throw)
	instance.run();                            // Loops in foreground forever
}
catch(const Exception &e)
{
	std::cerr << "Exception: " << e << std::endl;
	return -1;
}
