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
	static Adoc &configure(Adoc &doc);
	static Adoc configure(Chan &chan);

	DefaultConfig()
	{
		put("min_votes","1");
		put("min_yay","1");
		put("duration","15");
		put("plurality","0.51");
	}
};


class Vote
{
	Chans &chans;
	Users &users;

	Adoc cfg;                                   // Configuration of this vote
	time_t began;                               // Time vote was constructed
	std::string chan;                           // Name of the channel
	std::string issue;                          // "Issue" display name of the vote
	std::set<std::string> yay;                  // Accounts voting Yes
	std::set<std::string> nay;                  // Accounts voting No

  public:
	auto &get_cfg() const                       { return cfg;                                       }
	auto &get_chan() const                      { return chans.get(chan);                           }
	auto &get_began() const                     { return began;                                     }
	auto &get_issue() const                     { return issue;                                     }
	auto &get_yay() const                       { return yay;                                       }
	auto &get_nay() const                       { return nay;                                       }
	auto get_plurality() const                  { return cfg.get<float>("plurality",0.51);          }
	auto get_min_votes() const                  { return cfg.get<size_t>("min_votes",2);            }
	auto get_min_yay() const                    { return cfg.get<size_t>("min_yay",1);              }
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

	virtual void accepted()                     { get_chan() << "The yays have it!" << flush;       }
	virtual void declined()                     { get_chan() << "The nays have it!" << flush;       }
	virtual void validate()                     {} // If this throws the vote is canceled

  public:
	enum Stat { ADDED, CHANGED, ALREADY };

	Stat vote_yay(const User &user);
	Stat vote_nay(const User &user);

	void finish();                              // Called by the asynchronous deadline timer
	void start();                               // Called by Voting construction function

	Vote(Chans &chans, Users &users, Chan &chan, const std::string &issue, const Adoc &cfg = {});
	virtual ~Vote() = default;
};


inline
Vote::Vote(Chans &chans,
           Users &users,
           Chan &chan,
           const std::string &issue,
           const Adoc &cfg):
chans(chans),
users(users),
//cfg(cfg),
cfg(DefaultConfig::configure(chan)),
began(time(NULL)),
chan(chan.get_name()),
issue(issue)
{
	//DefaultConfig::configure(this->cfg);

}


inline
void Vote::start()
try
{
	validate();

	auto &chan = get_chan();
	chan << "Vote initiated! You have " << get_duration() << " seconds left to vote! ";
	chan << "Type: !vote yay or !vote nay" << flush;
}
catch(const Exception &e)
{
	auto &chan = get_chan();
	chan << "Vote is not valid: " << e << flush;
	throw;
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
		declined();
		return;
	}

	if(yay.size() < get_min_yay())
	{
		chan << "Failed to reach minimum number of yes votes: ";
		chan << yay.size() << " of " << get_min_yay() << " required." << flush;
		declined();
		return;
	}

	if(yay.size() < required())
	{
		chan << "Failed to pass. Yays: " << yay.size() << ". Nays: " << nay.size() << ". ";
		chan << "Required at least: " << required() << " yays." << flush;
		declined();
		return;
	}

	accepted();

	const auto t = tally();
	chan << "The vote passed with: " << t.first << ", against: " << t.second << " ";
	chan << "The yays have it!" << flush;
}
catch(const Exception &e)
{
	auto &chan = get_chan();
	chan << "The vote was not accepted in post-processing: " << e << flush;
	return;
}


inline
Vote::Stat Vote::vote_yay(const User &user)
{
	const bool erased = nay.erase(user.get_acct());
	const bool added = yay.emplace(user.get_acct()).second;
	return !added? ALREADY:
	       erased? CHANGED:
	               ADDED;
}


inline
Vote::Stat Vote::vote_nay(const User &user)
{
	const bool erased = yay.erase(user.get_acct());
	const bool added = nay.emplace(user.get_acct()).second;
	return !added? ALREADY:
	       erased? CHANGED:
	               ADDED;
}



inline
Adoc DefaultConfig::configure(Chan &chan)
{
	static const DefaultConfig default_config;

	Adoc cfg = chan.get("config");
	if(cfg.empty() || cfg.get_child("vote").empty())
	{
		cfg.add_child("vote",default_config);
		chan.set("config",cfg);
	}

	return cfg.get_child("vote");
}


inline
Adoc &DefaultConfig::configure(Adoc &vote_cfg)
{
	static const DefaultConfig default_config;

//	for(const auto &p : default_config)
//		if(!vote_cfg.count(p.first))
//			vote_cfg.put(*p);

	return vote_cfg;
}
