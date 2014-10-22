/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Sess
{
	std::mutex &mutex;                                 // Bot's main mutex or "session mutex"

	// Our constant data
	Opts opts;

	// libircclient
	Callbacks cbs;
	irc_session_t *sess;

	// Server data
	Server server;                                     // Filled at connection time
	std::set<std::string> caps;                        // registered extended capabilities
	std::string nickname;                              // NICK reply
	Mode mode;                                         // UMODE
	bool identified;                                   // Identified to services
	std::map<std::string,Mode> access;                 // Our channel access (/ns LISTCHANS)

	auto get()                                         { return sess;                               }
	auto get_cbs()                                     { return &cbs;                               }
	operator auto ()                                   { return get();                              }

	// Handler accesses
	friend class Bot;
	friend class NickServ;
	void set_nick(const std::string &nickname)         { this->nickname = nickname;                 }
	void delta_mode(const std::string &str)            { mode.delta(str);                           }
	void set_identified(const bool &identified)        { this->identified = identified;             }


  public:
	// mutable session mutex convenience access
	auto &get_mutex() const                            { return const_cast<std::mutex &>(mutex);    }
	auto &get_mutex()                                  { return mutex;                              }

	// Local data observers
	auto &get_opts() const                             { return opts;                               }

	// libircclient direct
	auto get_cbs() const                               { return &cbs;                               }
	auto get() const                                   { return sess;                               }
	operator auto () const                             { return get();                              }

	// Server data
	auto &get_server() const                           { return server;                             }
	auto &get_isupport() const                         { return get_server().isupport;              }
	auto &get_nick() const                             { return nickname;                           }
	auto &get_mode() const                             { return mode;                               }
	auto &get_access() const                           { return access;                             }
	auto isupport(const std::string &key) const        { return get_server().isupport(key);         }
	auto has_opt(const std::string &key) const         { return opts.get<bool>(key);                }
	bool has_cap(const std::string &cap) const         { return caps.count(cap);                    }
	bool is_desired_nick() const                       { return nickname == opts["nick"];           }
	auto &is_identified() const                        { return identified;                         }
	bool is_conn() const;

	// [SEND] libircclient call wrapper
	template<class F, class... A> void call(F&& f, A&&... a);
	template<class F, class... A> bool call(std::nothrow_t, F&& f, A&&... a);

	// [SEND] Raw send
	template<class... VA_LIST> void quote(const char *const &fmt, VA_LIST&&... ap);

	// [SEND] Services commands
	void nickserv(const std::string &str)              { quote("ns %s",str.c_str());                }
	void chanserv(const std::string &str)              { quote("cs %s",str.c_str());                }
	void memoserv(const std::string &str)              { quote("ms %s",str.c_str());                }
	void operserv(const std::string &str)              { quote("os %s",str.c_str());                }
	void botserv(const std::string &str)               { quote("bs %s",str.c_str());                }

	// [SEND] IRCv3 commands
	void authenticate(const std::string &str)          { quote("AUTHENTICATE %s",str.c_str());      }
	void cap_req(const std::string &cap)               { quote("CAP REQ :%s",cap.c_str());          }
	void cap_list()                                    { quote("CAP LIST");                         }
	void cap_end()                                     { quote("CAP END");                          }
	void cap_ls()                                      { quote("CAP LS");                           }

	// [SEND] Buddy commands
	template<class It> void ison(const It &begin, const It &end);
	void ison(const std::string &nick)                 { quote("ISON %s",nick.c_str());             }

	void monitor(const std::string &cmd)               { quote("MONITOR %s",cmd.c_str());           }
	template<class It> void monitor_add(const It &begin, const It &end);
	template<class It> void monitor_del(const It &begin, const It &end);
	void monitor_status()                              { monitor("S");                              }
	void monitor_clear()                               { monitor("C");                              }
	void monitor_list()                                { monitor("L");                              }

	void accept(const std::string &str)                { quote("ACCEPT %s",str.c_str());            }
	template<class It> void accept_del(const It &begin, const It &end);
	template<class It> void accept(const It &begin, const It &end);

	// [SEND] Content commands
	template<class It> void topics(const It &begin, const It &end, const std::string &server = "");
	void lusers(const std::string &mask = "", const std::string &server = "");
	void list(const std::string &server = "")          { quote("LIST %s",server.c_str());           }

	// [SEND] Baked commands
	void identify(const std::string &acct, const std::string &pass);
	void regain(const std::string &nick, const std::string &pass = "");
	void ghost(const std::string &nick, const std::string &pass);
	void identify();
	void regain();
	void ghost();

	// [SEND] Primary commands
	void help(const std::string &topic)                { quote("HELP %s",topic.c_str());            }
	void nick(const std::string &nick)                 { call(irc_cmd_nick,nick.c_str());           }
	void umode(const std::string &mode)                { call(irc_cmd_user_mode,mode.c_str());      }
	void umode()                                       { call(irc_cmd_user_mode,nullptr);           }
	void disconn()                                     { irc_disconnect(get());                     }
	void quit();                                       // Quit to server
	void conn();

	Sess(std::mutex &mutex, const Opts &opts, Callbacks &cbs, irc_session_t *const &sess = nullptr);
	Sess(const Sess &) = delete;
	Sess &operator=(const Sess &) = delete;
	~Sess() noexcept;

	friend std::ostream &operator<<(std::ostream &s, const Sess &sess);
};


inline
Sess::Sess(std::mutex &mutex,
           const Opts &opts,
           Callbacks &cbs,
           irc_session_t *const &sess):
mutex(mutex),
opts(opts),
cbs(cbs),
sess(sess? sess : irc_create_session(get_cbs())),
nickname(this->opts["nick"]),
identified(false)
{
	// Use the same global locale for each session for now.
	// Raise an issue if you have a case for this being a problem.
	irc::bot::locale = std::locale(opts["locale"].c_str());

}


inline
Sess::~Sess() noexcept
{
	if(sess)
		irc_destroy_session(get());
}


inline
void Sess::identify()
{
	identify(opts["ns-acct"],opts["ns-pass"]);
}


inline
void Sess::ghost()
{
	ghost(opts["nick"],opts["ns-pass"]);
}


inline
void Sess::regain()
{
	regain(opts["nick"],opts["ns-pass"]);
}


inline
void Sess::identify(const std::string &acct,
                    const std::string &pass)
{
	std::stringstream ss;
	ss << "identify" << " " << acct << " " << pass;
	nickserv(ss.str());
}


inline
void Sess::ghost(const std::string &nick,
                 const std::string &pass)
{
	std::stringstream gs;
	gs << "ghost" << " " << nick << " " << pass;
	nickserv(gs.str());
}


inline
void Sess::regain(const std::string &nick,
                  const std::string &pass)
{
	std::stringstream gs;
	gs << "regain" << " " << nick << " " << pass;
	nickserv(gs.str());
}


inline
void Sess::conn()
{
	call(irc_connect,
	     opts["host"].c_str(),
	     lex_cast<uint16_t>(opts["port"]),
	     opts["pass"].empty()? nullptr : opts["pass"].c_str(),
	     opts["nick"].c_str(),
	     opts["user"].c_str(),
	     opts["gecos"].c_str());
}


inline
void Sess::quit()
{
	call(irc_cmd_quit,"Alea iacta est.");
}


template<class It>
void Sess::ison(const It &begin,
                const It &end)
{
	std::stringstream ss;
	std::for_each(begin,end,[&ss]
	(const auto &s)
	{
		ss << s << " ";
	});

	quote("ISON :%s",ss.str().c_str());
}


template<class It>
void Sess::topics(const It &begin,
                  const It &end,
                  const std::string &server)
{
	std::stringstream ss;
	std::for_each(begin,end,[&ss]
	(const auto &s)
	{
		ss << s << ",";
	});

	quote("LIST %s %s",ss.str().c_str(),server.c_str());
}


template<class It>
void Sess::accept(const It &begin,
                  const It &end)
{
	std::stringstream ss;
	std::for_each(begin,end,[&ss]
	(const auto &s)
	{
		ss << s << ",";
	});

	accept(ss.str());
}


template<class It>
void Sess::accept_del(const It &begin,
                      const It &end)
{
	std::stringstream ss;
	std::for_each(begin,end,[&ss]
	(const auto &s)
	{
		ss << "-" << s << ",";
	});

	accept(ss.str());
}


template<class It>
void Sess::monitor_add(const It &begin,
                       const It &end)
{
	std::stringstream ss;
	ss << "+ ";

	std::for_each(begin,end,[&ss]
	(const auto &s)
	{
		ss << s << ",";
	});

	monitor(ss.str());
}


template<class It>
void Sess::monitor_del(const It &begin,
                       const It &end)
{
	std::stringstream ss;
	ss << "+ ";

	std::for_each(begin,end,[&ss]
	(const auto &s)
	{
		ss << s << ",";
	});

	monitor(ss.str());
}


inline
void Sess::lusers(const std::string &mask,
                  const std::string &server)
{
	quote("LUSERS %s %s",mask.c_str(),server.c_str());
}


inline
bool Sess::is_conn()
const
{
	return irc_is_connected(const_cast<decltype(sess)>(get()));
}


template<class... VA_LIST>
void Sess::quote(const char *const &fmt,
                 VA_LIST&&... ap)
{
	call(irc_send_raw,fmt,std::forward<VA_LIST>(ap)...);
}


template<class Func,
         class... Args>
void Sess::call(Func&& func,
                Args&&... args)
{
	static const uint32_t CALL_MAX_ATTEMPTS = 25;
	static constexpr std::chrono::milliseconds CALL_THROTTLE {750};

	// Loop to reattempt for certain library errors
	for(size_t i = 0; i < CALL_MAX_ATTEMPTS; i++) try
	{
		irc_call(get(),func,std::forward<Args>(args)...);
		return;
	}
	catch(const Exception &e)
	{
		// No more reattempts at the limit
		if(i >= CALL_MAX_ATTEMPTS - 1)
			throw;

		switch(e.code())
		{
			case LIBIRC_ERR_NOMEM:
				// Sent too much data without letting libirc recv(), we can throttle and try again.
				std::cerr << "call(): \033[1;33mthrottling send\033[0m"
				          << " (attempt: " << i << " timeout: " << CALL_THROTTLE.count() << ")"
				          << std::endl;

				std::this_thread::sleep_for(CALL_THROTTLE);
				continue;

			default:
				// No reattempt for everything else
				throw;
		}
	}
}


template<class Func,
         class... Args>
bool Sess::call(std::nothrow_t,
                Func&& func,
                Args&&... args)
{
	return irc_call(std::nothrow,get(),func,std::forward<Args>(args)...);
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Sess &ss)
{
	s << "Cbs:             " << ss.get_cbs() << std::endl;
	s << "Opts:            " << std::endl << ss.opts << std::endl;
	s << "irc_session_t:   " << ss.sess << std::endl;
	s << "server:          " << ss.server << std::endl;
	s << "nick:            " << ss.get_nick() << std::endl;
	s << "mode:            " << ss.mode << std::endl;

	s << "caps:            ";
	for(const auto &cap : ss.caps)
		s << "[" << cap << "]";
	s << std::endl;

	for(const auto &p : ss.access)
		s << "access:          " << p.second << "\t => " << p.first << std::endl;

	return s;
}
