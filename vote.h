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
	const Sess &sess;
	Chans &chans;
	Users &users;
	Logs &logs;

  public:
	using id_t = size_t;
	enum Stat                                   { ADDED, CHANGED,                                   };
	enum Ballot                                 { YEA, NAY,                                         };

  private:
	id_t id;                                    // Index id of this vote
	Adoc cfg;                                   // Configuration of this vote
	time_t began;                               // Time vote was constructed
	std::string chan;                           // Name of the channel
	std::string user;                           // $a name of initiating user
	std::string issue;                          // "Issue" input of the vote
	std::set<std::string> yea;                  // Accounts voting Yes
	std::set<std::string> nay;                  // Accounts voting No
	std::set<std::string> hosts;                // Hostnames that have voted
	std::set<std::string> vetoes;               // Accounts voting No with intent to veto

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
	auto &get_hosts() const                     { return hosts;                                     }
	auto &get_vetoes() const                    { return vetoes;                                    }
	auto num_vetoes() const                     { return vetoes.size();                             }
	auto elapsed() const                        { return time(NULL) - get_began();                  }
	auto remaining() const                      { return cfg.get<uint>("duration") - elapsed();     }
	auto tally() const -> std::pair<uint,uint>  { return {yea.size(),nay.size()};                   }
	auto total() const                          { return yea.size() + nay.size();                   }
	bool interceded() const;
	uint plurality() const;
	uint minimum() const;
	uint required() const;
	bool disabled() const;

	friend Locutor &operator<<(Locutor &l, const Vote &v);  // Appends formatted #ID to the stream
	bool enfranchised(const std::string &acct) const;
	bool qualified(const std::string &acct) const;
	Ballot position(const std::string &acct) const;         // Throws if user hasn't taken a position
	uint voted_host(const std::string &host) const;
	bool voted_acct(const std::string &acct) const;

	bool intercession(const User &user) const;
	bool enfranchised(const User &user) const   { return enfranchised(user.get_acct());             }
	bool qualified(const User &user) const      { return qualified(user.get_acct());                }
	Ballot position(const User &user) const     { return position(user.get_acct());                 }
	bool voted(const User &user) const;

  protected:
	static constexpr auto &flush = Locutor::flush;

	auto &get_sess() const                      { return sess;                                      }
	auto &get_users()                           { return users;                                     }
	auto &get_chans()                           { return chans;                                     }
	auto &get_logs()                            { return logs;                                      }

	// Subclass throws from these for abortions
	virtual void passed() {}
	virtual void failed() {}
	virtual void vetoed() {}
	virtual void canceled() {}
	virtual void proffer(const Ballot &b, User &user) {}
	virtual void starting() {}

  private:
	Stat cast(const Ballot &ballot, User &u);

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
