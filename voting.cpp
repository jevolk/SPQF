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
#include "vdb.h"
#include "votes.h"
#include "voting.h"


Voting::Voting(Vdb &vdb,
               Sess &sess,
               Chans &chans,
               Users &users,
               Logs &logs,
               Bot &bot):
sess(sess),
chans(chans),
users(users),
logs(logs),
bot(bot),
vdb(vdb),
thread(&Voting::worker,this)
{


}


Voting::~Voting() noexcept
{
	thread.join();

}


void Voting::cancel(const id_t &id,
                    const Chan &chan,
                    const User &user)
{
	Vote &vote = get(id);
	cancel(vote,chan,user);
}


void Voting::cancel(Vote &vote,
                    const Chan &chan,
                    const User &user)
{
	if(user.get_acct() != vote.get_user_acct())
		throw Exception() << "You can't cancel a vote by " << vote.get_user_acct() << ".";

	if(vote.total() > 1)
		throw Exception("You can't cancel after someone else has voted.");

	vote.cancel();
	del(vote.get_id());
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
		if(vote.remaining() <= 0 || vote.interceded())
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
		throw Exception("Could not delete any vote by this ID.");

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


Vote::id_t Voting::get_next_id()
const
{
	size_t i = 0;
	for(auto it = vdb.begin(); it; ++it,i++)
	{
		auto id = boost::lexical_cast<Vote::id_t>(it->first);
		if(id != i && !exists(id))
			break;
	}

	return i;
}


void Voting::for_each(const std::function<void (Vote &vote)> &closure)
{
	for(auto &p : votes)
		closure(*p.second);
}


void Voting::for_each(const User &user,
                      const std::function<void (Vote &vote)> &closure)
{
	auto pit = useridx.equal_range(user.get_acct());
	for(; pit.first != pit.second; ++pit.first) try
	{
		const auto &id = pit.first->second;
		closure(get(id));
	}
	catch(const Exception &e)
	{
		std::cerr << "Voting index user[" << user.get_acct() << "]: " << e << std::endl;
	}
}


void Voting::for_each(const Chan &chan,
                      const std::function<void (Vote &vote)> &closure)
{
	auto pit = chanidx.equal_range(chan.get_name());
	for(; pit.first != pit.second; ++pit.first) try
	{
		const auto &id = pit.first->second;
		closure(get(id));
	}
	catch(const Exception &e)
	{
		std::cerr << "Voting index chan[" << chan.get_name() << "]: " << e << std::endl;
	}
}


void Voting::for_each(const std::function<void (const Vote &vote)> &closure)
const
{
	for(const auto &p : votes)
		closure(*p.second);
}


void Voting::for_each(const User &user,
                      const std::function<void (const Vote &vote)> &closure)
const
{
	auto pit = useridx.equal_range(user.get_acct());
	for(; pit.first != pit.second; ++pit.first) try
	{
		const auto &id = pit.first->second;
		closure(get(id));
	}
	catch(const Exception &e)
	{
		std::cerr << "Voting index user[" << user.get_acct() << "]: " << e << std::endl;
	}
}


void Voting::for_each(const Chan &chan,
                      const std::function<void (const Vote &vote)> &closure)
const
{
	auto pit = chanidx.equal_range(chan.get_name());
	for(; pit.first != pit.second; ++pit.first) try
	{
		const auto &id = pit.first->second;
		closure(get(id));
	}
	catch(const Exception &e)
	{
		std::cerr << "Voting index chan[" << chan.get_name() << "]: " << e << std::endl;
	}
}


Vote &Voting::get(const id_t &id)
try
{
	return *votes.at(id);
}
catch(const std::out_of_range &e)
{
	throw Exception("There is no active vote with that ID.");
}


const Vote &Voting::get(const id_t &id)
const try
{
	return *votes.at(id);
}
catch(const std::out_of_range &e)
{
	throw Exception("There is no active vote with that ID.");
}


const Voting::id_t &Voting::get_id(const User &user)
const
{
	if(count(user) > 1)
		throw Exception("There are multiple votes initiated by this user. Specify an ID#.");

	const auto it = useridx.find(user.get_acct());
	if(it == useridx.end())
		throw Exception("There are no active votes initiated by this user.");

	return it->second;
}


const Voting::id_t &Voting::get_id(const Chan &chan)
const
{
	if(count(chan) > 1)
		throw Exception("There are multiple votes active in this channel. Specify an ID#.");

	const auto it = chanidx.find(chan.get_name());
	if(it == chanidx.end())
		throw Exception("There are no active votes in this channel.");

	return it->second;
}
