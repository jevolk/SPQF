/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct DefaultConfig : Adoc
{
	static uint configure(Adoc &doc);
	static Adoc configure(Chan &chan);

	DefaultConfig();
};


class Vote
{
  public:
	using id_t = size_t;
	enum Stat                                   { ADDED, CHANGED,                                   };
	enum Ballot                                 { YEA, NAY,                                         };

  private:
	const Sess &sess;
	Chans &chans;
	Users &users;
	Logs &logs;

	id_t id;                                    // Index id of this vote
	Adoc cfg;                                   // Configuration of this vote
	time_t began;                               // Time vote was constructed
	std::string chan;                           // Name of the channel
	std::string user;                           // $a name of initiating user
	std::string issue;                          // "Issue" input of the vote
	std::set<std::string> yea;                  // Accounts voting Yes
	std::set<std::string> nay;                  // Accounts voting No

  public:
	virtual const char *type() const            { return "";                                        }
	auto &get_id() const                        { return id;                                        }
	auto &get_cfg() const                       { return cfg;                                       }
	auto &get_chan_name() const                 { return chan;                                      }
	auto &get_user_acct() const                 { return user;                                      }
	auto &get_chan() const                      { return chans.get(get_chan_name());                }
	auto &get_began() const                     { return began;                                     }
	auto &get_issue() const                     { return issue;                                     }
	auto &get_yea() const                       { return yea;                                       }
	auto &get_nay() const                       { return nay;                                       }
	auto elapsed() const                        { return time(NULL) - get_began();                  }
	auto remaining() const                      { return cfg.get<uint>("duration") - elapsed();     }
	auto tally() const -> std::pair<uint,uint>  { return {yea.size(),nay.size()};                   }
	auto total() const                          { return yea.size() + nay.size();                   }
	uint plurality() const;
	uint minimum() const;
	uint required() const;
	bool disabled() const;

	friend Locutor &operator<<(Locutor &l, const Vote &v);    // Appends formatted #ID to channel stream

  protected:
	static constexpr auto &flush = Locutor::flush;

	auto &get_sess() const                      { return sess;                                      }
	auto &get_users()                           { return users;                                     }
	auto &get_chans()                           { return chans;                                     }
	auto &get_logs()                            { return logs;                                      }

	// Subclass throws from these for abortions
	virtual void passed() {}
	virtual void failed() {}
	virtual void canceled() {}
	virtual void proffer(const Ballot &b, User &user) {}
	virtual void starting() {}

	void enfranchise(const Ballot &b, User &user);
	Stat cast(const Ballot &ballot, User &user);

  public:
	// Called by Bot handlers
	void vote(const Ballot &ballot, User &user);

	// Called by the asynchronous Voting worker only
	void start();
	void finish();
	void cancel();

	Vote(const id_t &id, const Sess &sess, Chans &chans, Users &users, Logs &logs, Chan &chan, User &user, const std::string &issue, Adoc cfg = {});
	virtual ~Vote() = default;
};
