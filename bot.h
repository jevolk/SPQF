/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Bot
{
	Adb adb;
	Sess sess;
	Chans chans;
	Users users;

  public:
	const Adb &get_adb() const                              { return adb;                         }
	const Sess &get_sess() const                            { return sess;                        }
	const Chans &get_chans() const                          { return chans;                       }
	const Users &get_users() const                          { return users;                       }

	bool ready() const                                      { return get_sess().is_conn();        }
	const std::string &get_nick() const                     { return get_sess().get_nick();       }
	bool my_nick(const std::string &nick) const             { return get_nick() == nick;          }

  protected:
	Adb &get_adb()                                          { return adb;                         }
	Sess &get_sess()                                        { return sess;                        }
	Chans &get_chans()                                      { return chans;                       }
	Users &get_users()                                      { return users;                       }

	// [RECV] Handlers for user
	virtual void handle_privmsg(const Msg &m, User &u) {}
	virtual void handle_notice(const Msg &m, User &u) {}

	virtual void handle_chanmsg(const Msg &m, Chan &c, User &u) {}
	virtual void handle_cnotice(const Msg &m, Chan &c, User &u) {}
	virtual void handle_kick(const Msg &m, Chan &c, User &u) {}
	virtual void handle_part(const Msg &m, Chan &c, User &u) {}
	virtual void handle_join(const Msg &m, Chan &c, User &u) {}
	virtual void handle_mode(const Msg &m, Chan &c, User &u) {}
	virtual void handle_mode(const Msg &m, Chan &c) {}

  private:
	void log_handle(const Msg &m, const std::string &name,  const std::string &remarks = "") const;
	void log_handle(const Msg &m, const uint32_t &code, const std::string &remarks = "") const;

	// [RECV] Handlers update internal state first, then call user's handler^
	void handle_unhandled(const Msg &m, const std::string &name);
	void handle_unhandled(const Msg &m);

	void handle_bannedfromchan(const Msg &m);
	void handle_channelmodeis(const Msg &m);
	void handle_topicwhotime(const Msg &m);
	void handle_creationtime(const Msg &m);
	void handle_endofnames(const Msg &m);
	void handle_namreply(const Msg &m);
	void handle_quietlist(const Msg &m);
	void handle_banlist(const Msg &m);
	void handle_cnotice(const Msg &m);
	void handle_chanmsg(const Msg &m);
	void handle_topic(const Msg &m);
	void handle_mode(const Msg &m);
	void handle_kick(const Msg &m);
	void handle_part(const Msg &m);
	void handle_join(const Msg &m);

	void handle_ctcp_act(const Msg &m);
	void handle_ctcp_rep(const Msg &m);
	void handle_ctcp_req(const Msg &m);

	void handle_erroneusnickname(const Msg &m);
	void handle_nosuchnick(const Msg &m);
	void handle_whowasuser(const Msg &m);
	void handle_endofwhois(const Msg &m);
	void handle_whoischannels(const Msg &m);
	void handle_whoisaccount(const Msg &m);
	void handle_whoissecure(const Msg &m);
	void handle_whoisserver(const Msg &m);
	void handle_whoisidle(const Msg &m);
	void handle_whoisuser(const Msg &m);
	void handle_whospcrpl(const Msg &m);
	void handle_whoreply(const Msg &m);
	void handle_umodeis(const Msg &m);
	void handle_notice(const Msg &m);
	void handle_privmsg(const Msg &m);
	void handle_invite(const Msg &m);
	void handle_umode(const Msg &m);
	void handle_nick(const Msg &m);
	void handle_quit(const Msg &m);
	void handle_conn(const Msg &m);

  public:
	// Event/Handler input
	void operator()(const uint32_t &event, const char *const  &origin, const char **const &params, const size_t &count);
	void operator()(const char *const &event, const char *const &origin, const char **const &params, const size_t &count);

	// Run worker loop
	void join(const std::string &chan)                      { get_chans().join(chan);             }
	void quit()                                             { get_sess().quit();                  }
	void conn()                                             { get_sess().conn();                  }
	void run();

	Bot(void) = delete;
	Bot(const Ident &ident, irc_callbacks_t &cbs);
	Bot(Bot &&) = delete;
	Bot(const Bot &) = delete;
	Bot &operator=(Bot &&) = delete;
	Bot &operator=(const Bot &) = delete;
	~Bot(void) noexcept;

	friend std::ostream &operator<<(std::ostream &s, const Bot &bot);
};
