/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */

#include <chrono>

class Sess
{
	// Our constant data
	Ident ident;

	// libircclient
	irc_callbacks_t *cbs;
	irc_session_t *sess;

	// Server data
	Server server;                                     // Filled at connection time
	std::set<std::string> caps;                        // registered extended capabilities
	std::string nick;                                  // NICK reply
	Mode mode;                                         // UMODE

	irc_session_t *get()                               { return sess;                               }
	operator irc_session_t *()                         { return get();                              }

	// Bot handler access
	friend class Bot;
	void set_nick(const std::string &nick)             { this->nick = nick;                         }
	void delta_mode(const std::string &str)            { mode.delta(str);                           }
	void cap_req(const std::string &cap);              // IRCv3 CAP REQ
	void cap_end();                                    // IRCv3 CAP END

  public:
	// State observers
	const Ident &get_ident() const                     { return ident;                              }
	const irc_callbacks_t *get_cbs() const             { return cbs;                                }
	const irc_session_t *get() const                   { return sess;                               }
	operator const irc_session_t *() const             { return get();                              }

	// [SEND] libircclient call wrapper
	template<class F, class... A> void call(F&& f, A&&... a);
	template<class F, class... A> bool call(std::nothrow_t, F&& f, A&&... a);

	// [SEND] Raw send
	template<class... VA_LIST> void quote(const char *const &fmt, VA_LIST&&... ap);

	// IRC Observers
	const Server &get_server() const                   { return server;                             }
	const std::string &get_nick() const                { return nick;                               }
	const Mode &get_mode() const                       { return mode;                               }
	bool has_cap(const std::string &cap) const         { return caps.count(cap);                    }
	bool is_desired_nick() const                       { return nick == ident["nickname"];          }
	bool is_conn() const;

	// [SEND] IRC Controls
	void help(const std::string &topic);               // IRCd response goes to console
	void umode(const std::string &mode);               // Send umode update
	void umode();                                      // Request this->mode to be updated
	void cap_list();                                   // IRCv3 update our capabilities list
	void cap_ls();                                     // IRCv3 update server capabilities list
	void quit();                                       // Quit to server
	void disconn();
	void conn();

	Sess(const Ident &id, irc_callbacks_t &cbs, irc_session_t *const &sess);
	Sess(const Ident &id, irc_callbacks_t &cbs);
	Sess(const Sess &) = delete;
	Sess &operator=(const Sess &) = delete;
	~Sess() noexcept;

	friend std::ostream &operator<<(std::ostream &s, const Sess &sess);
};


inline
Sess::Sess(const Ident &ident,
           irc_callbacks_t &cbs):
Sess(ident,cbs,irc_create_session(&cbs))
{

}


inline
Sess::Sess(const Ident &ident,
           irc_callbacks_t &cbs,
           irc_session_t *const &sess):
ident(ident),
cbs(&cbs),
sess(sess),
nick(ident["nickname"])
{


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
	const Ident &id = get_ident();
	call(irc_connect,
	     id["hostname"].c_str(),
	     boost::lexical_cast<uint16_t>(id["port"]),
	     id["pass"].empty()? nullptr : id["pass"].c_str(),
	     id["nickname"].c_str(),
	     id["username"].c_str(),
	     id["fullname"].c_str());
}


inline
void Sess::disconn()
{
	irc_disconnect(get());
}


inline
void Sess::quit()
{
	call(irc_cmd_quit,"By all means must we fly; not with our feet, however, but with our hands.");
}


inline
void Sess::umode()
{
	call(irc_cmd_user_mode,nullptr);
}


inline
void Sess::umode(const std::string &mode)
{
	call(irc_cmd_user_mode,mode.c_str());
}


inline
void Sess::cap_ls()
{
	quote("CAP LS");
}


inline
void Sess::cap_list()
{
	quote("CAP LIST");
}


inline
void Sess::cap_req(const std::string &cap)
{
	quote("CAP REQ %s",cap.c_str());
}


inline
void Sess::cap_end()
{
	quote("CAP END");
}


inline
void Sess::help(const std::string &topic)
{
	quote("HELP %s",topic.c_str());
}


inline
bool Sess::is_conn()
const
{
	return irc_is_connected(const_cast<irc_session_t *>(get()));
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
	static const uint32_t CALL_MAX_ATTEMPTS = 50;
	static constexpr std::chrono::milliseconds CALL_THROTTLE {200};

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
	s << "Cbs:             " << ss.cbs << std::endl;
	s << "irc_session_t:   " << ss.sess << std::endl;
	s << "server:          " << ss.server << std::endl;
	s << "Ident:           " << ss.ident << std::endl;
	s << "nick:            " << ss.nick << std::endl;
	s << "mode:            " << ss.mode << std::endl;
	s << "caps:            ";
	for(const auto &cap : ss.caps)
		s << "[" << cap << "]";
	s << std::endl;

	return s;
}
