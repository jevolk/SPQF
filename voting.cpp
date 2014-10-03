/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include <condition_variable>
#include <thread>

#include "ircbot/bot.h"
#include "vote.h"
#include "votes.h"
#include "voting.h"


Voting::Voting(Bot &bot,
               Chans &chans,
               Users &users):
bot(bot),
chans(chans),
users(users),
thread(&Voting::worker,this)
{


}


void Voting::worker()
{
	while(1)
	{
		poll_motions();
		poll_votes();
		sleep();
	}
}


void Voting::sleep()
{
	//TODO: calculate next deadline. Recalculate on semaphore.
	std::this_thread::sleep_for(std::chrono::seconds(2));
}


void Voting::poll_votes()
{


}


void Voting::poll_motions()
{
	const std::unique_lock<std::mutex> lock(bot);
	for(auto it = motions.begin(); it != motions.end();)
	{
		const std::string &chan = it->first;
		Vote &vote = *it->second;
		if(vote.remaining() <= 0)
		{
			call_finish(vote);
			motions.erase(it++);
		}
		else ++it;
	}
}


void Voting::call_finish(Vote &vote)
noexcept try
{
	vote.finish();
}
catch(const std::exception &e)
{
	std::cerr << "Voting worker: UNHANDLED EXCEPTION: " << e.what() << std::endl;
	return;
}
