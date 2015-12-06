/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Vote : protected Acct
{
	static const std::string ARG_KEYED;
	static const std::string ARG_VALUED;
	static const std::set<std::string> ystr;
	static const std::set<std::string> nstr;

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
	time_t began;                               // Time vote was activated or 0
	time_t ended;                               // Time vote was closed or 0
	time_t expiry;                              // Time vote effects successfully expired.
	size_t quorum;                              // Quorum required
	std::string reason;                         // Reason for outcome
	std::string effect;                         // Effects of outcome
	std::set<std::string> yea;                  // Accounts voting Yes
	std::set<std::string> nay;                  // Accounts voting No
	std::set<std::string> veto;                 // Accounts voting No with intent to veto
	std::set<std::string> hosts;                // Hostnames that have voted

  public:
	using id_t = uint;
	enum Ballot                                 { YEA, NAY,                                         };
	enum Stat                                   { ADDED, CHANGED,                                   };

	auto get_id() const                         { return lex_cast<id_t>(id);                        }
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
	auto &get_expiry() const                    { return expiry;                                    }
	auto &get_reason() const                    { return reason;                                    }
	auto &get_effect() const                    { return effect;                                    }
	auto &get_yea() const                       { return yea;                                       }
	auto &get_nay() const                       { return nay;                                       }
	auto &get_hosts() const                     { return hosts;                                     }
	auto &get_veto() const                      { return veto;                                      }
	auto &get_quorum() const                    { return quorum;                                    }
	auto num_vetoes() const                     { return veto.size();                               }
	auto elapsed() const                        { return time(NULL) - get_began();                  }
	auto remaining() const                      { return secs_cast(cfg["duration"]) - elapsed();    }
	auto expires() const                        { return get_ended() + secs_cast(cfg["for"]);       }
	auto tally() const -> std::pair<uint,uint>  { return {yea.size(),nay.size()};                   }
	auto total() const                          { return yea.size() + nay.size();                   }
	bool disabled() const                       { return cfg.get<bool>("disable");                  }
	bool interceded() const;
	uint plurality() const;
	uint required() const;
	uint calc_quorum() const;

	operator Adoc() const;                                  // Serialize to Adoc/JSON
	friend Locutor &operator<<(Locutor &l, const Vote &v);  // Appends formatted #ID to the stream

	static bool is_ballot(const std::string &str);
	static Ballot ballot(const std::string &str);
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
	static constexpr auto flush = Locutor::flush;

	auto &get_sess() const                      { return *sess;                                     }
	auto &get_users() const                     { return *users;                                    }
	auto &get_chans() const                     { return *chans;                                    }
	auto &get_logs() const                      { return *logs;                                     }

	auto &get_sess()                            { return *sess;                                     }
	auto &get_users()                           { return *users;                                    }
	auto &get_chans()                           { return *chans;                                    }
	auto &get_logs()                            { return *logs;                                     }

	void set_issue(const std::string &issue)    { this->issue = issue;                              }
	void set_reason(const std::string &reason)  { this->reason = reason;                            }
	void set_effect(const std::string &effect)  { this->effect = effect;                            }
	void set_began()                            { time(&began);                                     }
	void set_ended()                            { time(&ended);                                     }
	void set_expiry()                           { time(&expiry);                                    }
	void set_quorum(const uint &quorum)         { this->quorum = quorum;                            }

	void announce_failed_required();
	void announce_failed_quorum();
	void announce_canceled();
	void announce_vetoed();
	void announce_passed();
	void announce_starting();

	// One-time internal events                 // Subclass throws from these for abortions at any time.
	virtual void passed() {}                    // Performs effects after successful vote
	virtual void failed() {}                    // Performs effects after failed vote
	virtual void vetoed() {}                    // Performs effects after vetoed vote
	virtual void canceled() {}                  // Performs effects after canceled vote
	virtual void starting() {}                  // Called after ctor, before broadcast to chan

	Stat cast(const Ballot &b, const User &u);

  public:
	// Various events while this vote is active.
	virtual void event_vote(User &u, const Ballot &b);
	virtual void event_nick(User &u, const std::string &old) {}
	virtual void event_notice(User &u, const std::string &text) {}
	virtual void event_privmsg(User &u, const std::string &text) {}
	virtual void event_chanmsg(User &u, const std::string &text) {}
	virtual void event_chanmsg(User &u, Chan &c, const std::string &text) {}
	virtual void event_cnotice(User &u, const std::string &text) {}
	virtual void event_cnotice(User &u, Chan &c, const std::string &text) {}

	// Various/events                           // (possibly while vote is not active)
	virtual void valid(const Adoc &cfg) const;  // Checks validity of the cfg document
	virtual void expired() {}                   // Reverts the effects after "cfg.for" time

	// Main controls used by Voting / Praetor
	void save()                                 { Acct::set(*this);                                 }
	void start();
	void finish();
	void cancel();
	void expire();

	// Deserialization ctor
	Vote(const std::string &type,               // Dummy argument to match main ctor for ...'s
	     const id_t &id,
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
