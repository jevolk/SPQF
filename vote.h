/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


enum class Ballot
{
	YEA,
	NAY,
};

enum class Stat
{
	ADDED,
	CHANGED,
};

using id_t = uint;
using Tally = std::pair<uint,uint>;

extern const std::set<std::string> ystr;
extern const std::set<std::string> nstr;

bool is_ballot(const std::string &str);
Ballot ballot(const std::string &str);
std::ostream &operator<<(std::ostream &s, const Ballot &ballot);

bool has_access(const Chan &chan, const User &user, const Mode &mode);
bool has_mode(const Chan &chan, const User &user, const Mode &mode);

bool enfranchised(const Adoc &cfg, const Chan &chan, const User &user, time_t began = 0);
bool qualified(const Adoc &cfg, const Chan &chan, const User &user, time_t began = 0);
bool intercession(const Adoc &cfg, const Chan &chan, const User &user);
bool speaker(const Adoc &cfg, const Chan &chan, const User &user);

uint calc_quorum(const Adoc &cfg, const Chan &chan, time_t began = 0);
uint calc_plurality(const Adoc &cfg, const Tally &tally);
uint calc_required(const Adoc &cfg, const Tally &tally);

class Vote : protected Acct
{
	static const std::string ARG_KEYED;
	static const std::string ARG_VALUED;

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
	std::string reason;                         // Reason for failure; no reason is passed vote
	std::string effect;                         // Effects of outcome; only filled once effective
	std::set<std::string> yea;                  // Accounts voting Yes
	std::set<std::string> nay;                  // Accounts voting No
	std::set<std::string> veto;                 // Accounts voting No with intent to veto
	std::set<std::string> hosts;                // Hostnames that have voted

  public:
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
	auto elapsed() const                        { return time(NULL) - get_began();                  }
	auto remaining() const                      { return secs_cast(cfg["duration"]) - elapsed();    }
	auto expires() const                        { return get_ended() + secs_cast(cfg["for"]);       }
	auto tally() const -> Tally                 { return { yea.size(), nay.size() };                }
	auto total() const                          { return yea.size() + nay.size();                   }
	bool enabled() const                        { return cfg.get("enable",false);                   }
	bool interceded() const;
	bool prejudiced() const;

	Ballot position(const std::string &acct) const;         // Throws if user hasn't taken a position
	uint voted_host(const std::string &host) const;
	bool voted_acct(const std::string &acct) const;

	Ballot position(const User &user) const     { return position(user.get_acct());                 }
	bool voted_host(const User &user) const     { return voted_host(user.get_acct());               }
	bool voted_acct(const User &user) const     { return voted_acct(user.get_acct());               }
	bool voted(const User &user) const;

  protected:
	void set_cfg(const Adoc &cfg)               { this->cfg = cfg;                                  }
	void set_issue(const std::string &issue)    { this->issue = issue;                              }
	void set_reason(const std::string &reason)  { this->reason = reason;                            }
	void set_effect(const std::string &effect)  { this->effect = effect;                            }
	void set_began()                            { time(&began);                                     }
	void set_ended()                            { time(&ended);                                     }
	void set_expiry()                           { time(&expiry);                                    }
	void set_quorum(const uint &quorum)         { this->quorum = quorum;                            }

	void announce_ballot_reject(User &user, const std::string &reason);
	void announce_ballot_accept(User &user, const Stat &stat);
	void announce_failed_required();
	void announce_failed_quorum();
	void announce_canceled();
	void announce_vetoed();
	void announce_passed();
	void announce_starting();

	// Type specific overrides                  // Subclass throws from these for abortions at any time.
	virtual void passed() {}                    // After successful vote
	virtual void failed() {}                    // After failed vote
	virtual void vetoed() {}                    // After vetoed vote
	virtual void canceled() {}                  // After canceled vote
	virtual void effective() {}                 // After successful vote (before passed), or with prejudice
	virtual void starting() {}                  // Called after ctor, before broadcast to chan

  public:
	virtual void expired() {}                   // After cfg.for time expires

	Stat cast(const Ballot &b, const User &u);
	virtual void event_vote(User &u, const Ballot &b);
	virtual void event_nick(User &u, const std::string &old) {}
	virtual void event_notice(User &u, const std::string &text) {}

	operator Adoc() const;                      // Serialize to Adoc/JSON

	// Main controls used by Voting / Praetor
	void save()                                 { Acct::set(*this);                                 }
	void start();
	void finish();
	void cancel();
	void expire();

	// Deserialization ctor
	Vote(const std::string &type,               // Dummy argument to match main ctor for ...'s
	     const id_t &id,
	     Adb &adb);

	// Motion ctor (main ctor)
	Vote(const std::string &type,
	     const id_t &id,
	     Adb &adb,
	     Chan &chan,
	     User &user,
	     const std::string &issue,
	     const Adoc &cfg = {});

	virtual ~Vote() = default;

	friend Locutor &operator<<(Locutor &l, const Vote &v);  // Appends formatted #ID to the stream
};
