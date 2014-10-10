/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Voting
{
	using id_t = Vote::id_t;

	Bot &bot;
	Sess &sess;
	Chans &chans;
	Users &users;
	Logs &logs;

	std::map<id_t, std::unique_ptr<Vote>> votes;     // Standing votes  : id => vote
	std::multimap<std::string, id_t> chanidx;        // Index of votes  : chan => id
	std::multimap<std::string, id_t> useridx;        // Index of votes  : acct => id
	std::condition_variable sem;                     // Notify worker of new work

  public:
	auto has_vote(const Chan &chan) const -> bool    { return chanidx.count(chan.get_name());   }
	auto has_vote(const User &user) const -> bool    { return useridx.count(user.get_acct());   }
	auto has_vote(const id_t &id) const -> bool      { return votes.count(id);                  }
	auto num_votes(const Chan &chan) const           { return chanidx.count(chan.get_name());   }
	auto num_votes(const User &user) const           { return useridx.count(user.get_acct());   }
	auto num_votes() const                           { return votes.size();                     }

	const id_t &get_id(const Chan &chan) const;      // Throws if more than one vote in channel
	const id_t &get_id(const User &chan) const;      // Throws if more than one vote for user
	const Vote &get(const id_t &id) const;
	const Vote &get(const Chan &chan) const          { return get(get_id(chan));                }
	const Vote &get(const User &user) const          { return get(get_id(user));                }
	Vote &get(const id_t &id);
	Vote &get(const Chan &chan)                      { return get(get_id(chan));                }
	Vote &get(const User &user)                      { return get(get_id(user));                }

	void for_each(const Chan &chan, const std::function<void (const Vote &vote)> &closure) const;
	void for_each(const User &user, const std::function<void (const Vote &vote)> &closure) const;
	void for_each(const std::function<void (const Vote &vote)> &closure) const;
	void for_each(const Chan &chan, const std::function<void (Vote &vote)> &closure);
	void for_each(const User &user, const std::function<void (Vote &vote)> &closure);
	void for_each(const std::function<void (Vote &vote)> &closure);

  private:
	void del(const decltype(votes.begin()) &it);
	void del(const id_t &id);

	void call_finish(Vote &vote) noexcept;
	void poll_votes();
	void sleep();
	void worker();
	std::thread thread;

  public:
	void cancel(Vote &vote, const Chan &chan, const User &user);
	void cancel(const id_t &id, const Chan &chan, const User &user);
	template<class Vote, class... Args> Vote &motion(Args&&... args);

	Voting(Bot &bot, Sess &sess, Chans &chans, Users &users, Logs &logs);
	Voting(const Voting &) = delete;
	Voting &operator=(const Voting &) = delete;
	~Voting() noexcept;
};


template<class Vote,
         class... Args>
Vote &Voting::motion(Args&&... args)
try
{
	id_t id; do
	{
		id = rand() % 999;
	}
	while(has_vote(id));

	const auto iit = votes.emplace(id,std::make_unique<Vote>(id,sess,chans,users,logs,std::forward<Args>(args)...));
	const bool &inserted = iit.second;

	if(inserted) try
	{
		using limits = std::numeric_limits<size_t>;

		Vote &vote = dynamic_cast<Vote &>(*iit.first->second);
		const std::string &chan_name = vote.get_chan_name();
		const std::string &acct_name = vote.get_user_acct();
		const Adoc &cfg = vote.get_cfg();

		chanidx.emplace(chan_name,id);
		useridx.emplace(acct_name,id);

		if(chanidx.count(chan_name) > cfg.get("max_active",limits::max()))
			throw Exception("Too many active votes for this channel.");

		if(useridx.count(acct_name) > cfg.get("max_per_user",limits::max()))
			throw Exception("Too many active votes started by you on this channel.");

		if(!vote.enfranchised(acct_name))
			throw Exception("You are not yet enfranchised in this channel.");

		if(!vote.qualified(acct_name))
			throw Exception("You have not been participating enough to start a vote.");

		vote.start();
		sem.notify_one();
		return vote;
	}
	catch(const std::exception &e)
	{
		del(id);
		throw;
	}
	else throw Exception("Failed to track this vote (internal error)");
}
catch(const Exception &e)
{
	std::stringstream ss;
	ss << "Vote is not valid: " << e;
	throw Exception(ss.str());
}
