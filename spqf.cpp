/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */

#include <dlfcn.h>
#include <signal.h>
#include "ircbot/bot.h"
using namespace irc::bot;


// Pointer to instance for signals
std::unique_ptr<Bot> bot;

// Synchronization for some signals etc
std::mutex mutex;
std::condition_variable cond;
bool hangup, interrupt;


static
void handle_hup()
{
	const std::lock_guard<decltype(mutex)> lock(mutex);
	if(hangup)
		return;

	hangup = true;
	cond.notify_all();
}


static
void handle_int()
{
	const std::lock_guard<decltype(mutex)> lock(mutex);
	if(interrupt)
		return;

	interrupt = true;
	cond.notify_all();
}


static
void handle_sig(const int sig)
{
	switch(sig)
	{
		case SIGHUP:     handle_hup();     break;
		case SIGINT:     handle_int();     break;
		case SIGTERM:    exit(0);          break;
	}
}


int main(int argc, char **argv) try
{
	srand(getpid());

	// Setup defaults
	Opts opts;
	opts["logdir"] = "logs";
	opts["logging"] = "true";
	opts["database"] = "true";
	opts["connect"] = "true";

	// Parse command line
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

	bot.reset(new Bot(opts));             // Create instance of the bot
	signal(SIGHUP,&handle_sig);           // Register handler for hangup
	signal(SIGINT,&handle_sig);           // Register handler for ctrl-c
	signal(SIGTERM,&handle_sig);          // Register handler for term
	(*bot)(Bot::BACKGROUND);

	while(!interrupt)
	{
		using mod_call_t = void (*)(Bot *);

		const auto module(dlopen("./respub.so",RTLD_NOW|RTLD_LOCAL));
		if(!module)
			throw Assertive("Fatal dlopen() error: ") << dlerror();

		const auto module_init(reinterpret_cast<mod_call_t>(dlsym(module,"module_init")));
		{
			const auto err(dlerror());
			if(!module_init || err)
				throw Assertive("Fatal dlsym() error: ") << err;
		}

		const auto module_fini(reinterpret_cast<mod_call_t>(dlsym(module,"module_fini")));
		{
			const auto err(dlerror());
			if(!module_fini || err)
				throw Assertive("Fatal dlsym() error: ") << err;
		}

		module_init(bot.get());
		{
			std::unique_lock<decltype(mutex)> lock(mutex);
			cond.wait(lock,[]{ return hangup || interrupt; });
			hangup = false;
		}
		module_fini(bot.get());

		if(dlclose(module) != 0)
			throw Assertive("Fatal dlclose() error: ") << dlerror();
	}

	bot->quit();
	recvq::ios.poll();
}
catch(const std::exception &e)
{
	std::cerr << "Exception: " << e.what() << std::endl;
	return -1;
}
