/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Ident
{
	std::string nick;
	std::string host;
	uint16_t port;

	std::vector<std::string> autojoin;

	friend std::ostream &operator<<(std::ostream &s, const Ident &id);
};


class Sess
{
	Ident ident;
	irc_callbacks_t *cbs;
	irc_session_t *sess;
	std::string nick;
	Mode mode;

	irc_session_t *get()                               { return sess;                               }
	operator irc_session_t *()                         { return get();                              }

	// Bot handler access
	friend class Bot;
	void set_nick(const std::string &nick)             { this->nick = nick;                         }
	void delta_mode(const std::string &str)            { mode.delta(str);                           }

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
	const std::string &get_nick() const                { return nick;                               }
	const Mode &get_mode() const                       { return mode;                               }
	bool is_desired_nick() const                       { return nick == get_ident().nick;           }
	bool is_conn() const;

	// [SEND] IRC Controls
	void umode(const std::string &mode);               // Send umode update
	void umode();                                      // Request this->mode to be updated
	void quit();                                       // Quit to server
	void disconn();
	void conn();

	Sess(const Ident &id, irc_callbacks_t &cbs, irc_session_t *const &sess);
	Sess(const Ident &id, irc_callbacks_t &cbs);
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
nick(ident.nick)
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
	const std::string &host = get_ident().host;
	const std::string &nick = get_ident().nick;
	const uint16_t &port = get_ident().port;
	call(irc_connect,host.c_str(),port,nullptr,nick.c_str(),nick.c_str(),nick.c_str());
}


inline
void Sess::disconn()
{
	irc_disconnect(get());
}


inline
void Sess::quit()
{
	call(irc_cmd_quit,"K-Lined");
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
	::irc_call(get(),func,std::forward<Args>(args)...);
}


template<class Func,
         class... Args>
bool Sess::call(std::nothrow_t,
                Func&& func,
                Args&&... args)
{
	return ::irc_call(std::nothrow,get(),func,std::forward<Args>(args)...);
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Sess &ss)
{
	s << "Ident:           " << ss.ident << std::endl;
	s << "Cbs:             " << ss.cbs << std::endl;
	s << "irc_session_t:   " << ss.sess << std::endl;
	s << "nick:            " << ss.nick << std::endl;
	s << "mode:            " << ss.mode << std::endl;
	return s;
}



inline
std::ostream &operator<<(std::ostream &s,
                         const Ident &id)
{
	s << "host: " << id.host;
	s << " port: " << id.port;
	s << " nick: " << id.nick;
	return s;
}
