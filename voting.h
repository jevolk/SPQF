/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Voting
{
	using id_t = Vote::id_t;

	Sess &sess;
	Chans &chans;
	Users &users;
	Logs &logs;
	Bot &bot;
	Vdb &vdb;
	Praetor &praetor;
	std::mutex mutex;                                // mutex for sem
	std::condition_variable sem;                     // Notify worker of new work
	std::atomic<bool> interrupted;                   // Worker exits on interrupted state
	std::atomic<bool> initialized;                   // Voting cannot begin until initialized
	std::map<id_t, std::unique_ptr<Vote>> votes;     // Standing votes  : id => vote
	std::multimap<std::string, id_t> chanidx;        // Index of votes  : chan => id
	std::multimap<std::string, id_t> useridx;        // Index of votes  : acct => id

  public:
	auto exists(const Chan &chan) const -> bool      { return chanidx.count(chan.get_name());   }
	auto exists(const User &user) const -> bool      { return useridx.count(user.get_acct());   }
	auto exists(const id_t &id) const -> bool        { return votes.count(id);                  }
	auto count(const Chan &chan) const               { return chanidx.count(chan.get_name());   }
	auto count(const User &user) const               { return useridx.count(user.get_acct());   }
	auto count() const                               { return votes.size();                     }

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
	id_t get_next_id() const;
	void worker_wait_init();
	template<class Duration> void worker_sleep(Duration&& duration);
	std::unique_ptr<Vote> del(const decltype(votes.begin()) &it);
	std::unique_ptr<Vote> del(const id_t &id);
	void valid_motion(Vote &vote);  // throws out

	void call_finish(Vote &vote) noexcept;
	void poll_votes();
	void poll_init();
	void poll_sleep();
	void poll_worker();
	std::thread poll_thread;

	void remind_votes();
	void remind_sleep();
	void remind_worker();
	std::thread remind_thread;

	id_t eligible_last_vote(const Chan &chan, const std::string &nick, Vdb::Terms terms);
	void eligible_add(Chan &chan);
	void eligible_add();
	void eligible_worker();
	std::thread eligible_thread;

  public:
	void cancel(Vote &vote, const Chan &chan, const User &user);
	void cancel(const id_t &id, const Chan &chan, const User &user);
	template<class Vote, class... Args> Vote &motion(Args&&... args);

	Voting(Sess &sess, Chans &chans, Users &users, Logs &logs, Bot &bot, Vdb &vdb, Praetor &praetor);
	Voting(const Voting &) = delete;
	Voting &operator=(const Voting &) = delete;
	~Voting() noexcept;
};


template<class Vote,
         class... Args>
Vote &Voting::motion(Args&&... args)
try
{
	if(!initialized.load(std::memory_order_consume))
		throw Exception("Voting not yet initialized: try again in a few minutes");

	const id_t id(get_next_id());
	const auto iit(votes.emplace(id,std::make_unique<Vote>(id,
	                                                       vdb,
	                                                       sess,
	                                                       chans,
	                                                       users,
	                                                       logs,
	                                                       std::forward<Args>(args)...)));
	if(iit.second) try
	{
		auto &ptr(iit.first->second);
		auto &vote(dynamic_cast<Vote &>(*ptr));
		chanidx.emplace(vote.get_chan_name(),id);
		useridx.emplace(vote.get_user_acct(),id);
		valid_motion(vote);
		vote.start();
		sem.notify_one();
		return vote;
	}
	catch(const std::exception &e)
	{
		del(id);
		throw;
	}
	else throw Assertive("Failed to track this vote");
}
catch(const Exception &e)
{
	throw Exception() << "New motion is not valid: " << e;
}
catch(const std::exception &e)
{
	throw Assertive(e.what());
}
