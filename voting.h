/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Voting
{
	using id_t = size_t;

	Bot &bot;
	Chans &chans;
	Users &users;
	std::map<std::string, std::unique_ptr<Vote>> motions;   // Quick motions   : channel => vote
	std::map<id_t, std::unique_ptr<Vote>> votes;            // Standing votes  : id => vote
	std::multimap<std::string, size_t> voteidx;             // Index of votes  : chan => id
	std::condition_variable sem;                            // Notify worker of new work

  public:
	auto has_motion(const std::string &chan) const -> bool  { return motions.count(chan);          }
	auto has_vote(const id_t &id) const -> bool             { return votes.count(id);              }

	auto num_motions() const                                { return motions.size();               }
	auto num_votes() const                                  { return votes.size();                 }
	auto num_votes(const std::string &chan) const           { return voteidx.count(chan);          }

	auto &get(const id_t &id) const;
	auto &get(const Chan &chan) const;

  private:
	void call_finish(Vote &vote) noexcept;
	void poll_motions();
	void poll_votes();
	void sleep();
	void worker();
	std::thread thread;

  public:
	template<class Vote, class... Args> id_t vote(Chan &chan, Args&&... args);
	template<class Vote, class... Args> void motion(Chan &chan, Args&&... args);

	void cancel(const id_t &id);
	void cancel(const Chan &chan);

	Voting(Bot &bot, Chans &chans, Users &users);
};


inline
void Voting::cancel(const Chan &chan)
{
	//auto &vote = get(chan);
	//vote.cancel();
	motions.erase(chan.get_name());
}


inline
void cancel(const id_t &id)
{


}


template<class Vote,
         class... Args>
void Voting::motion(Chan &chan,
                    Args&&... args)
try
{
	const std::string &name = chan.get_name();
	const auto iit = motions.emplace(name,std::make_unique<Vote>(chans,users,chan,std::forward<Args>(args)...));
	const bool &inserted = iit.second;

	if(inserted) try
	{
		auto &vote = *iit.first->second;
		vote.start();
		sem.notify_one();
	}
	catch(const std::exception &e)
	{
		motions.erase(name);
		throw;
	}
	else throw Exception("A motion is already active for this channel");
}
catch(const Exception &e)
{
	chan << "Vote is not valid: " << e << Chan::flush;
	return;
}


template<class Vote,
         class... Args>
Voting::id_t Voting::vote(Chan &chan,
                          Args&&... args)
try
{
	id_t id; do
	{
		id = rand() % 9999;
	}
	while(has_vote(id));

	const auto iit = motions.emplace(id,std::make_unique<Vote>(chans,users,chan,std::forward<Args>(args)...));
	const bool &inserted = iit.second;

	if(inserted) try
	{
		const scope notify([&]{ sem.notify_one(); });
		auto &vote = *iit.first->second;
		vote.start();
		sem.notify_one();
	}
	catch(const std::exception &e)
	{
		votes.erase(id);
		throw;
	}

	return id;
}
catch(const Exception &e)
{
	chan << "Vote is not valid: " << e << Chan::flush;
	return;
}


inline
auto &Voting::get(const id_t &id)
const try
{
	return *votes.at(id);
}
catch(const std::out_of_range &e)
{
	throw Exception("There is no active vote with that ID.");
}


inline
auto &Voting::get(const Chan &chan)
const try
{
	return *motions.at(chan.get_name());
}
catch(const std::out_of_range &e)
{
	throw Exception("There are no active votes on this channel.");
}
