/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Vote : protected Acct
{
	Sess *sess;
	Chans *chans;
	Users *users;
	Logs *logs;

	std::string id;                             // Index ID of vote (stored as string for Acct db)
	std::string type;                           // Type name of this vote
	std::string chan;                           // Name of the channel
	std::string nick;                           // Nick of initiating user (note: don't trust)
	std::string acct;                           // $a name of initiating user
	std::string issue;                          // "Issue" input of the vote
	Adoc cfg;                                   // Configuration of this vote
	time_t began;                               // Time vote was constructed
	time_t ended;                               // Time vote was closed
	std::string reason;                         // Reason for outcome
	std::set<std::string> yea;                  // Accounts voting Yes
	std::set<std::string> nay;                  // Accounts voting No
	std::set<std::string> veto;                 // Accounts voting No with intent to veto
	std::set<std::string> hosts;                // Hostnames that have voted

  public:
	using id_t = uint;
	enum Ballot                                 { YEA, NAY,                                         };
	enum Stat                                   { ADDED, CHANGED,                                   };

	auto get_id() const                         { return boost::lexical_cast<id_t>(id);             }
	auto &get_type() const                      { return type;                                      }
	auto &get_chan_name() const                 { return chan;                                      }
	auto &get_user_acct() const                 { return acct;                                      }
	auto &get_user_nick() const                 { return nick;                                      }
	auto &get_chan() const                      { return chans->get(get_chan_name());               }
	auto &get_user() const                      { return users->get(get_user_nick());               }
	auto &get_issue() const                     { return issue;                                     }
	auto &get_cfg() const                       { return cfg;                                       }
	auto &get_began() const                     { return began;                                     }
	auto &get_ended() const                     { return ended;                                     }
	auto &get_reason() const                    { return reason;                                    }
	auto &get_yea() const                       { return yea;                                       }
	auto &get_nay() const                       { return nay;                                       }
	auto &get_hosts() const                     { return hosts;                                     }
	auto &get_veto() const                      { return veto;                                      }
	auto num_vetoes() const                     { return veto.size();                               }
	auto elapsed() const                        { return time(NULL) - get_began();                  }
	auto remaining() const                      { return cfg.get<time_t>("duration") - elapsed();   }
	auto tally() const -> std::pair<uint,uint>  { return {yea.size(),nay.size()};                   }
	auto total() const                          { return yea.size() + nay.size();                   }
	bool disabled() const                       { return cfg.get<bool>("disable");                  }
	bool interceded() const;
	uint plurality() const;
	uint minimum() const;
	uint required() const;

	operator Adoc() const;                                  // Serialize to Adoc/JSON
	friend Locutor &operator<<(Locutor &l, const Vote &v);  // Appends formatted #ID to the stream

	Ballot position(const std::string &acct) const;         // Throws if user hasn't taken a position
	uint voted_host(const std::string &host) const;
	bool voted_acct(const std::string &acct) const;

	Ballot position(const User &user) const     { return position(user.get_acct());                 }
	bool voted_host(const User &user) const     { return voted_host(user.get_acct());               }
	bool voted_acct(const User &user) const     { return voted_acct(user.get_acct());               }

	bool has_access(const User &user, const Mode &flags) const;
	bool has_mode(const User &user, const Mode &flags) const;

	bool intercession(const User &user) const;
	bool enfranchised(const User &user) const;
	bool qualified(const User &user) const;
	bool speaker(const User &user) const;
	bool voted(const User &user) const;

  protected:
	static constexpr auto &flush = Locutor::flush;

	auto &get_sess() const                      { return *sess;                                     }
	auto &get_users() const                     { return *users;                                    }
	auto &get_chans() const                     { return *chans;                                    }
	auto &get_logs() const                      { return *logs;                                     }

	auto &get_sess()                            { return *sess;                                     }
	auto &get_users()                           { return *users;                                    }
	auto &get_chans()                           { return *chans;                                    }
	auto &get_logs()                            { return *logs;                                     }

	void set_reason(const std::string &reason)  { this->reason = reason;                            }
	void set_began()                            { time(&began);                                     }
	void set_ended()                            { time(&ended);                                     }

	// Subclass throws from these for abortions
	virtual void passed() {}
	virtual void failed() {}
	virtual void vetoed() {}
	virtual void canceled() {}
	virtual void proffer(const Ballot &b, User &user) {}
	virtual void starting() {}

	Stat cast(const Ballot &b, User &u);

  public:
	void save()                                 { Acct::set(*this);                                 }
	void vote(const Ballot &b, User &u);
	void start();
	void finish();
	void cancel();

	// Deserialization ctor
	Vote(const id_t &id,
	     Adb &adb,
	     Sess *const &sess     = nullptr,
	     Chans *const &chans   = nullptr,
	     Users *const &users   = nullptr,
	     Logs *const &logs     = nullptr);

	// Motion ctor (main ctor)
	Vote(const std::string &type,
	     const id_t &id,
	     Adb &adb,
	     Sess &sess,
	     Chans &chans,
	     Users &users,
	     Logs &logs,
	     Chan &chan,
	     User &user,
	     const std::string &issue,
	     const Adoc &cfg = {});

	virtual ~Vote() = default;
};
