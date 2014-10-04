/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


extern const char *const help_vote;             // help.cpp
extern const char *const help_vote_config;      // help.cpp
extern const char *const help_vote_kick;        // help.cpp
extern const char *const help_vote_mode;        // help.cpp


struct DefaultConfig : public Adoc
{
	static size_t configure(Adoc &doc);
	static Adoc configure(Chan &chan);

	DefaultConfig()
	{
		// config.vote
		put("min_votes",1);
		put("min_yay",1);
		put("min_turnout",0.00);
		put("duration",30);
		put("plurality",0.51);

		// config.vote.kick
		put("kick.if_away",1);
	}
};


class Vote
{
	Chans &chans;
	Users &users;

	Adoc cfg;                                   // Configuration of this vote
	time_t began;                               // Time vote was constructed
	std::string chan;                           // Name of the channel
	std::string user;                           // Initiating user ($a name)
	std::string issue;                          // "Issue" input of the vote
	std::set<std::string> yay;                  // Accounts voting Yes
	std::set<std::string> nay;                  // Accounts voting No

  public:
	enum Stat                                   { ADDED, CHANGED,                                   };
	enum Ballot                                 { YAY, NAY,                                         };

	auto &get_cfg() const                       { return cfg;                                       }
	auto &get_chan() const                      { return chans.get(chan);                           }
	auto &get_user() const                      { return user;                                      }
	auto &get_began() const                     { return began;                                     }
	auto &get_issue() const                     { return issue;                                     }
	auto &get_yay() const                       { return yay;                                       }
	auto &get_nay() const                       { return nay;                                       }
	auto get_plurality() const                  { return cfg.get<float>("plurality");               }
	auto get_min_votes() const                  { return cfg.get<size_t>("min_votes");              }
	auto get_min_yay() const                    { return cfg.get<size_t>("min_yay");                }
	auto get_min_turnout() const                { return cfg.get<size_t>("min_turnout");            }
	auto get_duration() const                   { return cfg.get<time_t>("duration");               }
	auto elapsed() const                        { return time(NULL) - get_began();                  }
	auto remaining() const                      { return get_duration() - elapsed();                }
	auto tally() const -> std::pair<uint,uint>  { return {yay.size(),nay.size()};                   }
	auto total() const                          { return yay.size() + nay.size();                   }
	auto plurality() const -> size_t            { return ceil(float(total()) * get_plurality());    }
	auto minimum() const                        { return std::max(get_min_votes(),get_min_yay());   }
	auto required() const                       { return std::max(get_min_yay(),plurality());       }

  protected:
	static constexpr auto &flush = Locutor::flush;

	auto &get_users()                           { return users;                                     }
	auto &get_chans()                           { return chans;                                     }
	auto &get_chan()                            { return chans.get(chan);                           }

	// Subclass throws from these for abortions
	virtual void passed()                       { get_chan() << "The yays have it!" << flush;       }
	virtual void failed()                       { get_chan() << "The nays have it!" << flush;       }
	virtual void canceled() {}
	virtual void proffer(const Ballot &b, User &u) {}
	virtual void starting() {}

  public:
	// Called by Bot handlers
	Stat vote(const Ballot &ballot, User &user);

	// Called by the asynchronous Voting worker only
	void cancel();
	void finish();
	void start();

	Vote(Chans &chans, Users &users, Chan &chan, User &user, const std::string &issue, Adoc cfg = {});
	virtual ~Vote() = default;
};


inline
Vote::Vote(Chans &chans,
           Users &users,
           Chan &chan,
           User &user,
           const std::string &issue,
           Adoc cfg):
chans(chans),
users(users),
cfg([&]() -> Adoc
{
	if(cfg.empty())
		return DefaultConfig::configure(chan);

	DefaultConfig::configure(cfg);
	return cfg;
}()),
began(time(NULL)),
chan(chan.get_name()),
user(user.get_acct()),
issue(issue)
{
	if(!user.is_logged_in())
		throw Exception("You must be registered with nickserv to create a vote.");
}


inline
void Vote::start()
{
	starting();

	auto &chan = get_chan();
	chan << "Vote initiated! You have " << get_duration() << " seconds left to vote! ";
	chan << "Type: !vote yay or !vote nay" << flush;
}


inline
void Vote::finish()
try
{
	auto &chan = get_chan();

	if(total() < get_min_votes())
	{
		chan << "Failed to reach minimum number of votes: ";
		chan << total() << " of " << get_min_votes() << " required." << flush;
		failed();
		return;
	}

	if(yay.size() < get_min_yay())
	{
		chan << "Failed to reach minimum number of yes votes: ";
		chan << yay.size() << " of " << get_min_yay() << " required." << flush;
		failed();
		return;
	}

	if(yay.size() < required())
	{
		chan << "Failed to pass. Yays: " << yay.size() << ". Nays: " << nay.size() << ". ";
		chan << "Required at least: " << required() << " yays." << flush;
		failed();
		return;
	}

	passed();

	const auto t = tally();
	chan << "The vote passed with: " << t.first << ", against: " << t.second << " ";
	chan << "The yays have it!" << flush;
}
catch(const Exception &e)
{
	auto &chan = get_chan();
	chan << "The vote was rejected: " << e << flush;
	return;
}


inline
void Vote::cancel()
{
	canceled();
	get_chan() << "The vote has been canceled." << flush;
}


inline
Vote::Stat Vote::vote(const Ballot &ballot,
                      User &user)
{
	if(!user.is_logged_in())
		throw Exception("You must be registered with nickserv to vote.");

	proffer(ballot,user);

	switch(ballot)
	{
		case YAY:
			if(!yay.emplace(user.get_acct()).second)
				throw Exception("You have already voted yay in this election.");

			return nay.erase(user.get_acct())? CHANGED : ADDED;

		case NAY:
			if(!nay.emplace(user.get_acct()).second)
				throw Exception("You have already voted nay in this election.");

			return yay.erase(user.get_acct())? CHANGED : ADDED;

		default:
			throw Exception("Ballot type not accepted");
	}
}


inline
Adoc DefaultConfig::configure(Chan &chan)
{
	Adoc cfg = chan.get("config.vote");

	if(configure(cfg))
	{
		Adoc chan_cfg;
		chan_cfg.put_child("config.vote",cfg);
		chan.set(chan_cfg);
	}

	return cfg;
}


inline
size_t DefaultConfig::configure(Adoc &cfg)
{
	static const DefaultConfig default_config;

	const std::function<size_t (const Adoc &def, Adoc &cfg)> recurse = [&]
	(const Adoc &def, Adoc &cfg) -> size_t
	{
		size_t ret = 0;
		for(const auto &pair : def)
		{
			const std::string &key = pair.first;
			const auto &subtree = pair.second;

			if(!subtree.empty())
			{
				Adoc sub = cfg.get_child(key,Adoc());
				ret += recurse(subtree,sub);
				cfg.put_child(key,sub);
			}
			else if(!cfg.has_child(key))
			{
				cfg.put_child(key,subtree);
				ret++;
			}
		}

		return ret;
	};

	return recurse(default_config,cfg);
}
