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
	Ident ident;
	std::locale locale;

	// libircclient
	Callbacks cbs;
	irc_session_t *sess;

	// Server data
	Server server;                                     // Filled at connection time
	std::set<std::string> caps;                        // registered extended capabilities
	std::string _nick;                                 // NICK reply
	Mode mode;                                         // UMODE
	bool identified;                                   // Identified to services
	std::map<std::string,Mode> access;                 // Our channel access (/ns LISTCHANS)

	auto get()                                         { return sess;                               }
	auto get_cbs()                                     { return &cbs;                               }
	operator auto ()                                   { return get();                              }

	// Handler accesses
	friend class Bot;
	friend class NickServ;
	void set_nick(const std::string &nick)             { this->_nick = nick;                        }
	void delta_mode(const std::string &str)            { mode.delta(str);                           }
	void set_identified(const bool &identified)        { this->identified = identified;             }


  public:
	// mutable session mutex convenience access
	auto &get_mutex() const                            { return const_cast<std::mutex &>(mutex);    }
	auto &get_mutex()                                  { return mutex;                              }

	// Local data observers
	auto &get_ident() const                            { return ident;                              }
	auto &get_locale() const                           { return locale;                             }

	// libircclient direct
	auto get_cbs() const                               { return &cbs;                               }
	auto get() const                                   { return sess;                               }
	operator auto () const                             { return get();                              }

	// Server data
	auto &get_server() const                           { return server;                             }
	auto &get_nick() const                             { return _nick;                              }
	auto &get_mode() const                             { return mode;                               }
	auto &is_identified() const                        { return identified;                         }
	auto get_access() const                            { return access;                             }
	bool has_cap(const std::string &cap) const         { return caps.count(cap);                    }
	bool is_desired_nick() const                       { return _nick == ident["nick"];             }
	bool is_conn() const;

	// [SEND] libircclient call wrapper
	template<class F, class... A> void call(F&& f, A&&... a);
	template<class F, class... A> bool call(std::nothrow_t, F&& f, A&&... a);

	// [SEND] Raw send
	template<class... VA_LIST> void quote(const char *const &fmt, VA_LIST&&... ap);

	// [SEND] IRCv3 commands
	void authenticate(const std::string &str)          { quote("AUTHENTICATE %s",str.c_str());      }
	void cap_req(const std::string &cap)               { quote("CAP REQ :%s",cap.c_str());          }
	void cap_list()                                    { quote("CAP LIST");                         }
	void cap_end()                                     { quote("CAP END");                          }
	void cap_ls()                                      { quote("CAP LS");                           }

	// [SEND] Primary commands
	void nickserv(const std::string &str)              { quote("ns %s",str.c_str());                }
	void chanserv(const std::string &str)              { quote("cs %s",str.c_str());                }
	void memoserv(const std::string &str)              { quote("ms %s",str.c_str());                }
	void operserv(const std::string &str)              { quote("os %s",str.c_str());                }
	void botserv(const std::string &str)               { quote("bs %s",str.c_str());                }
	void help(const std::string &topic)                { quote("HELP %s",topic.c_str());            }
	void nick(const std::string &nick)                 { call(irc_cmd_nick,nick.c_str());           }
	void umode(const std::string &mode)                { call(irc_cmd_user_mode,mode.c_str());      }
	void umode()                                       { call(irc_cmd_user_mode,nullptr);           }
	void quit();                                       // Quit to server
	void disconn()                                     { irc_disconnect(get());                     }
	void conn();

	// [SEND] Baked commands
	void ghost(const std::string &nick, const std::string &pass);
	void regain(const std::string &nick, const std::string &pass = "");
	void identify(const std::string &acct, const std::string &pass);
	void ghost();
	void regain();
	void identify();

	Sess(std::mutex &mutex, const Ident &id, Callbacks &cbs, irc_session_t *const &sess = nullptr);
	Sess(const Sess &) = delete;
	Sess &operator=(const Sess &) = delete;
	~Sess() noexcept;

	friend std::ostream &operator<<(std::ostream &s, const Sess &sess);
};


inline
Sess::Sess(std::mutex &mutex,
           const Ident &ident,
           Callbacks &cbs,
           irc_session_t *const &sess):
mutex(mutex),
ident(ident),
locale(this->ident["locale"].c_str()),
cbs(cbs),
sess(sess? sess : irc_create_session(get_cbs())),
_nick(this->ident["nick"]),
identified(false)
{


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
	const Ident &id = get_ident();
	identify(ident["ns-acct"],ident["ns-pass"]);
}


inline
void Sess::ghost()
{
	const Ident &id = get_ident();
	ghost(ident["nick"],ident["ns-pass"]);
}


inline
void Sess::regain()
{
	const Ident &id = get_ident();
	regain(ident["nick"],ident["ns-pass"]);
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
	const Ident &id = get_ident();
	call(irc_connect,
	     id["host"].c_str(),
	     boost::lexical_cast<uint16_t>(id["port"]),
	     id["pass"].empty()? nullptr : id["pass"].c_str(),
	     id["nick"].c_str(),
	     id["user"].c_str(),
	     id["gecos"].c_str());
}


inline
void Sess::quit()
{
	call(irc_cmd_quit,"Alea iacta est.");
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
	s << "irc_session_t:   " << ss.sess << std::endl;
	s << "server:          " << ss.server << std::endl;
	s << "Ident:           " << std::endl << ss.ident << std::endl;
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
