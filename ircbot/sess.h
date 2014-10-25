/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Sess
{
	std::mutex &mutex;                                 // Bot's main mutex or "session mutex"
	Opts opts;
	Callbacks cbs;
	irc_session_t *sess;
	SendQ sendq;
	Server server;                                     // Filled at connection time
	std::set<std::string> caps;                        // registered extended capabilities
	std::string nickname;                              // NICK reply
	Mode mymode;                                       // UMODE
	bool identified;                                   // Identified to services
	std::map<std::string,Mode> access;                 // Our channel access (/ns LISTCHANS)

	auto get()                                         { return sess;                               }
	auto get_cbs()                                     { return &cbs;                               }
	operator auto ()                                   { return get();                              }

  private:
	// Handler accesses
	friend class Bot;
	friend class NickServ;
	void set_nick(const std::string &nickname)         { this->nickname = nickname;                 }
	void delta_mode(const std::string &str)            { mymode.delta(str);                         }
	void set_identified(const bool &identified)        { this->identified = identified;             }

  public:
	auto &get_mutex() const                            { return const_cast<std::mutex &>(mutex);    }
	auto &get_opts() const                             { return opts;                               }
	auto get() const                                   { return sess;                               }
	operator auto () const                             { return get();                              }
	auto get_cbs() const                               { return &cbs;                               }
	auto &get_sendq() const                            { return sendq;                              }
	auto &get_server() const                           { return server;                             }
	auto &get_isupport() const                         { return get_server().isupport;              }
	auto &get_nick() const                             { return nickname;                           }
	auto &get_mode() const                             { return mymode;                             }
	auto &get_access() const                           { return access;                             }
	auto isupport(const std::string &key) const        { return get_server().isupport(key);         }
	auto has_opt(const std::string &key) const         { return opts.get<bool>(key);                }
	bool has_cap(const std::string &cap) const         { return caps.count(cap);                    }
	bool is_desired_nick() const                       { return nickname == opts["nick"];           }
	auto &is_identified() const                        { return identified;                         }
	bool is_conn() const;

	SendQ &get_sendq()                                 { return sendq;                              }

	void umode(const std::string &m);
	void umode();
	void disconn()                                     { irc_disconnect(get());                     }
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
sendq(this->sess),
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
void Sess::conn()
{
	irc_call(get(),
	         irc_connect,
	         opts["host"].c_str(),
	         lex_cast<uint16_t>(opts["port"]),
	         opts["pass"].empty()? nullptr : opts["pass"].c_str(),
	         opts["nick"].c_str(),
	         opts["user"].c_str(),
	         opts["gecos"].c_str());
}


inline
void Sess::umode()
{
	sendq << "MODE " << get_nick() << sendq.flush;
}


inline
void Sess::umode(const std::string &str)
{
	sendq << "MODE " << get_nick() << " " << str << sendq.flush;
}


inline
bool Sess::is_conn()
const
{
	return irc_is_connected(const_cast<decltype(sess)>(get()));
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Sess &ss)
{
	s << "Cbs:             " << ss.get_cbs() << std::endl;
	s << "Opts:            " << std::endl << ss.get_opts() << std::endl;
	s << "irc_session_t:   " << ss.get() << std::endl;
	s << "server:          " << ss.get_server() << std::endl;
	s << "nick:            " << ss.get_nick() << std::endl;
	s << "mode:            " << ss.get_mode() << std::endl;

	s << "caps:            ";
	for(const auto &cap : ss.caps)
		s << "[" << cap << "]";
	s << std::endl;

	for(const auto &p : ss.access)
		s << "access:          " << p.second << "\t => " << p.first << std::endl;

	return s;
}
