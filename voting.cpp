/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


// libircbot irc::bot::
#include "ircbot/bot.h"

// SPQF
using namespace irc::bot;
#include "vote.h"
#include "votes.h"
#include "voting.h"


Voting::Voting(Bot &bot,
               Sess &sess,
               Chans &chans,
               Users &users,
               Logs &logs):
bot(bot),
sess(sess),
chans(chans),
users(users),
logs(logs),
thread(&Voting::worker,this)
{


}


Voting::~Voting() noexcept
{
	thread.join();

}


void Voting::worker()
{
	while(1)
	{
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
	const std::unique_lock<Bot> lock(bot);
	for(auto it = votes.begin(); it != votes.end();)
	{
		Vote &vote = *it->second;
		if(vote.remaining() <= 0)
		{
			call_finish(vote);
			del(it++);
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


void Voting::del(const id_t &id)
{
	const auto vit = votes.find(id);
	if(vit == votes.end())
		throw Exception("Failed to delete vote by ID");

	del(vit);
}


void Voting::del(const decltype(votes.begin()) &it)
{
	const Vote &vote = *it->second;
	const Vote::id_t &id = it->first;
	const auto deindex = [&id]
	(auto &map, const auto &ent)
	{
		const auto pit = map.equal_range(ent);
		const auto it = std::find_if(pit.first,pit.second,[&id]
		(const auto &it)
		{
			const id_t &idx = it.second;
			return idx == id;
		});

		if(it != pit.second)
			map.erase(it);
	};

	deindex(chanidx,vote.get_chan_name());
	deindex(useridx,vote.get_user_acct());
	votes.erase(it);
}
