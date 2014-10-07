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

	std::map<id_t, std::unique_ptr<Vote>> votes;        // Standing votes  : id => vote
	std::multimap<std::string, id_t> chanidx;           // Index of votes  : chan => id
	std::multimap<std::string, id_t> useridx;           // Index of votes  : acct => id
	std::condition_variable sem;                        // Notify worker of new work

  public:
	auto has_vote(const Chan &chan) const -> bool       { return chanidx.count(chan.get_name());   }
	auto has_vote(const User &user) const -> bool       { return useridx.count(user.get_acct());   }
	auto has_vote(const id_t &id) const -> bool         { return votes.count(id);                  }

	auto num_votes(const Chan &chan) const              { return chanidx.count(chan.get_name());   }
	auto num_votes(const User &user) const              { return useridx.count(user.get_acct());   }
	auto num_votes() const                              { return votes.size();                     }

	const id_t &get_id(const Chan &chan) const;         // Throws if more than one vote in channel
	const id_t &get_id(const User &chan) const;         // Throws if more than one vote for user

	const Vote &get(const id_t &id) const;
	const Vote &get(const Chan &chan) const             { return get(get_id(chan));                }
	const Vote &get(const User &user) const             { return get(get_id(user));                }

  private:
	void del(const decltype(votes.begin()) &it);
	void del(const id_t &id);

	void call_finish(Vote &vote) noexcept;
	void poll_motions();
	void poll_votes();
	void sleep();
	void worker();
	std::thread thread;

  public:
	void for_each(const Chan &chan, const std::function<void (const Vote &vote)> &closure) const;
	void for_each(const User &user, const std::function<void (const Vote &vote)> &closure) const;
	void for_each(const std::function<void (const Vote &vote)> &closure) const;

	void for_each(const Chan &chan, const std::function<void (Vote &vote)> &closure);
	void for_each(const User &user, const std::function<void (Vote &vote)> &closure);
	void for_each(const std::function<void (Vote &vote)> &closure);

	Vote &get(const id_t &id);
	Vote &get(const Chan &chan)                         { return get(get_id(chan));               }
	Vote &get(const User &user)                         { return get(get_id(user));               }

	void cancel(const id_t &id, const Chan &chan, const User &user);

	// Initiate a vote
	template<class Vote, class... Args> Vote &motion(Args&&... args);

	Voting(Bot &bot, Sess &sess, Chans &chans, Users &users);
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

	const auto iit = votes.emplace(id,std::make_unique<Vote>(id,sess,chans,users,std::forward<Args>(args)...));
	const bool &inserted = iit.second;

	if(inserted) try
	{
	    using limits = std::numeric_limits<size_t>;

		Vote &vote = dynamic_cast<Vote &>(*iit.first->second);
		chanidx.emplace(vote.get_chan_name(),id);
		useridx.emplace(vote.get_user_acct(),id);

		const Adoc &cfg = vote.get_cfg();
		const Chan &chan = vote.get_chan();
		const User &user = vote.get_user();

		if(vote.disabled())
			throw Exception("Votes of this type are disabled by the configuration.");

		if(num_votes(chan) > cfg.get("max_active",limits::max()))
			throw Exception("Too many active votes for this channel.");

		if(num_votes(user) > cfg.get("max_per_user",limits::max()))
			throw Exception("Too many active votes started by you on this channel.");

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


inline
void Voting::cancel(const id_t &id,
                    const Chan &chan,
                    const User &user)
{
	Vote &vote = get(id);

	if(user.get_acct() != vote.get_user_acct())
	{
		std::stringstream ss;
		ss << "You can't cancel a vote by " << vote.get_user_acct() << ".";
		throw Exception(ss.str());
	}

	if(vote.total() > 0)
		throw Exception("You can't cancel after a vote has been cast.");

	vote.cancel();
	del(id);
}


inline
void Voting::for_each(const std::function<void (Vote &vote)> &closure)
{
	for(const auto &p : votes)
	{
		Vote &vote = *p.second;
		closure(vote);
	}
}


inline
void Voting::for_each(const User &user,
                      const std::function<void (Vote &vote)> &closure)
{
	auto pit = useridx.equal_range(user.get_acct());
	for(; pit.first != pit.second; ++pit.first) try
	{
		const auto &id = pit.first->second;
		Vote &vote = *votes.at(id);
		closure(vote);
	}
	catch(const std::out_of_range &e)
	{
		std::cerr << "Voting index user[" << user.get_acct() << "]: "
		          << e.what() << std::endl;
	}
}


inline
void Voting::for_each(const Chan &chan,
                      const std::function<void (Vote &vote)> &closure)
{
	auto pit = chanidx.equal_range(chan.get_name());
	for(; pit.first != pit.second; ++pit.first) try
	{
		const auto &id = pit.first->second;
		Vote &vote = *votes.at(id);
		closure(vote);
	}
	catch(const std::out_of_range &e)
	{
		std::cerr << "Voting index chan[" << chan.get_name() << "]: "
		          << e.what() << std::endl;
	}
}


inline
void Voting::for_each(const std::function<void (const Vote &vote)> &closure)
const
{
	for(const auto &p : votes)
	{
		const Vote &vote = *p.second;
		closure(vote);
	}
}


inline
void Voting::for_each(const User &user,
                      const std::function<void (const Vote &vote)> &closure)
const
{
	auto pit = useridx.equal_range(user.get_acct());
	for(; pit.first != pit.second; ++pit.first) try
	{
		const auto &id = pit.first->second;
		const Vote &vote = *votes.at(id);
		closure(vote);
	}
	catch(const std::out_of_range &e)
	{
		std::cerr << "Voting index user[" << user.get_acct() << "]: "
		          << e.what() << std::endl;
	}
}


inline
void Voting::for_each(const Chan &chan,
                      const std::function<void (const Vote &vote)> &closure)
const
{
	auto pit = chanidx.equal_range(chan.get_name());
	for(; pit.first != pit.second; ++pit.first) try
	{
		const auto &id = pit.first->second;
		const Vote &vote = *votes.at(id);
		closure(vote);
	}
	catch(const std::out_of_range &e)
	{
		std::cerr << "Voting index chan[" << chan.get_name() << "]: "
		          << e.what() << std::endl;
	}
}


inline
Vote &Voting::get(const id_t &id)
try
{
	return *votes.at(id);
}
catch(const std::out_of_range &e)
{
	throw Exception("There is no active vote with that ID.");
}


inline
const Vote &Voting::get(const id_t &id)
const try
{
	return *votes.at(id);
}
catch(const std::out_of_range &e)
{
	throw Exception("There is no active vote with that ID.");
}


inline
const Voting::id_t &Voting::get_id(const User &user)
const
{
	if(num_votes(user) > 1)
		throw Exception("There are multiple votes initiated by this user. Specify an ID#.");

	const auto it = useridx.find(user.get_acct());
	if(it == useridx.end())
		throw Exception("There are no active votes initiated by this user.");

	const auto &id = it->second;
	return id;
}


inline
const Voting::id_t &Voting::get_id(const Chan &chan)
const
{
	if(num_votes(chan) > 1)
		throw Exception("There are multiple votes active in this channel. Specify an ID#.");

	const auto it = chanidx.find(chan.get_name());
	if(it == chanidx.end())
		throw Exception("There are no active votes in this channel.");

	const auto &id = it->second;
	return id;
}
