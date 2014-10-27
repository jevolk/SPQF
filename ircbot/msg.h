/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#define SPQF_FMT(type, ...)  namespace type { enum { __VA_ARGS__ }; }


inline namespace fmt
{
SPQF_FMT( WELCOME,           TEXT                                                                            )
SPQF_FMT( YOURHOST,          SELFNAME, TEXT                                                                  )
SPQF_FMT( CREATED,           SELFNAME, TEXT                                                                  )
SPQF_FMT( MYINFO,            SELFNAME, SERVNAME, VERSION,  USERMODS, CHANMODS, CHANPARM                      )
SPQF_FMT( CAP,               SELFNAME, COMMAND,  CAPLIST                                                     )
SPQF_FMT( ACCOUNT,           ACCTNAME,                                                                       )
SPQF_FMT( NICK,              NICKNAME                                                                        )
SPQF_FMT( ISON,              SELFNAME, NICKLIST                                                              )
SPQF_FMT( LIST,              SELFNAME, CHANNAME, USERNUM,  TOPIC                                             )
SPQF_FMT( QUIT,              REASON                                                                          )
SPQF_FMT( JOIN,              CHANNAME, ACCTNAME, EGECOS                                                      )
SPQF_FMT( PART,              CHANNAME, REASON                                                                )
SPQF_FMT( MODE,              CHANNAME, DELTASTR                                                              )
SPQF_FMT( AWAY,              SELFNAME, NICKNAME, MESSAGE                                                     )
SPQF_FMT( UMODE,             DELTASTR                                                                        )
SPQF_FMT( UMODEIS,           SELFNAME, DELTASTR                                                              )
SPQF_FMT( INVITE,            SELFNAME, CHANNAME,                                                             )
SPQF_FMT( NOTOPIC,           CHANNAME, INFO,                                                                 )
SPQF_FMT( TOPIC,             CHANNAME, TEXT,                                                                 )
SPQF_FMT( RPLTOPIC,          SELFNAME, CHANNAME, TEXT,                                                       )
SPQF_FMT( CHANNELMODEIS,     NICKNAME, CHANNAME, DELTASTR                                                    )
SPQF_FMT( CHANOPRIVSNEEDED,  NICKNAME, CHANNAME, REASON                                                      )
SPQF_FMT( NICKNAMEINUSE,     ASTERISK, NICKNAME, REASON                                                      )
SPQF_FMT( USERNOTINCHANNEL,  SELFNAME, NICKNAME, CHANNAME, REASON                                            )
SPQF_FMT( USERONCHANNEL,     SELFNAME, NICKNAME, CHANNAME, REASON                                            )
SPQF_FMT( ALREADYONCHAN,     SELFNAME, CHANNAME, REASON                                                      )
SPQF_FMT( KICK,              CHANNAME, TARGET,   REASON                                                      )
SPQF_FMT( CHANMSG,           CHANNAME, TEXT                                                                  )
SPQF_FMT( CNOTICE,           CHANNAME, TEXT                                                                  )
SPQF_FMT( PRIVMSG,           NICKNAME, TEXT                                                                  )
SPQF_FMT( CACTION,           CHANNAME, TEXT                                                                  )
SPQF_FMT( ACTION,            SELFNAME, TEXT                                                                  )
SPQF_FMT( NOTICE,            SELFNAME, TEXT                                                                  )
SPQF_FMT( NAMREPLY,          NICKNAME, TYPE,     CHANNAME, NAMELIST                                          )
SPQF_FMT( WHOREPLY,          SELFNAME, CHANNAME, USERNAME, HOSTNAME, SERVNAME, NICKNAME, FLAGS,    ADDL      )
SPQF_FMT( WHOISUSER,         SELFNAME, NICKNAME, USERNAME, HOSTNAME, ASTERISK, REALNAME                      )
SPQF_FMT( WHOISIDLE,         SELFNAME, NICKNAME, SECONDS,  SIGNON,   REMARKS                                 )
SPQF_FMT( WHOISSECURE,       SELFNAME, NICKNAME, REMARKS,                                                    )
SPQF_FMT( WHOISACCOUNT,      SELFNAME, NICKNAME, ACCTNAME, REMARKS                                           )
SPQF_FMT( CREATIONTIME,      SELFNAME, CHANNAME, TIME,                                                       )
SPQF_FMT( TOPICWHOTIME,      SELFNAME, CHANNAME, MASK,     TIME,                                             )
SPQF_FMT( BANLIST,           SELFNAME, CHANNAME, BANMASK,  OPERATOR, TIME,                                   )
SPQF_FMT( EXCEPTLIST,        SELFNAME, CHANNAME, MASK,     OPERATOR, TIME,                                   )
SPQF_FMT( INVITELIST,        SELFNAME, CHANNAME, MASK,     OPERATOR, TIME,                                   )
SPQF_FMT( QUIETLIST,         SELFNAME, CHANNAME, MODECODE, BANMASK,  OPERATOR, TIME,                         )
SPQF_FMT( MONLIST,           SELFNAME, NICKLIST                                                              )
SPQF_FMT( MONONLINE,         SELFNAME, MASKLIST                                                              )
SPQF_FMT( MONOFFLINE,        SELFNAME, NICKLIST                                                              )
SPQF_FMT( ACCEPTLIST,        SELFNAME, /* ... */                                                             )
SPQF_FMT( ACCEPTEXIST,       SELFNAME, NICKNAME, REASON                                                      )
SPQF_FMT( ACCEPTNOT,         SELFNAME, NICKNAME, REASON                                                      )
SPQF_FMT( ACCEPTFULL,        SELFNAME                                                                        )
SPQF_FMT( KNOCK,             CHANNAME, CHANNAM2, MASK,     REASON                                            )
}


class Msg
{
	using Params = std::vector<std::string>;

	uint32_t code;
	std::string name;
	std::string origin;
	Params params;

  public:
	auto &get_code() const                                  { return code;                      }
	auto &get_name() const                                  { return name;                      }
	auto &get_origin() const                                { return origin;                    }
	auto &get_params() const                                { return params;                    }
	auto num_params() const                                 { return get_params().size();       }

	auto &get(const size_t &i) const                        { return get_params().at(i);        }
	auto &operator[](const size_t &i) const;                // returns empty str for outofrange
	template<class R> R get(const size_t &i) const;         // throws for range or bad cast
	template<class R> R operator[](const size_t &i) const   { return get<R>(i);                 }
	auto begin() const                                      { return params.begin();            }
	auto end() const                                        { return params.end();              }

	std::string get_nick() const;
	std::string get_host() const;
	bool from_server() const                                { return get_nick() == get_host();  }
	bool from_nickserv() const                              { return get_nick() == "NickServ";  }
	bool from_chanserv() const                              { return get_nick() == "ChanServ";  }
	bool from_memoserv() const                              { return get_nick() == "MemoServ";  }
	bool from_operserv() const                              { return get_nick() == "OperServ";  }
	bool from_botserv() const                               { return get_nick() == "BotServ";   }

	Msg(const uint32_t &code, const std::string &origin, const Params &params);
	Msg(const uint32_t &code, const char *const &origin, const char **const &params, const size_t &count);
	Msg(const std::string &name, const std::string &origin, const Params &params);
	Msg(const char *const &name, const char *const &origin, const char **const &params, const size_t &count);

	friend std::ostream &operator<<(std::ostream &s, const Msg &m);
};


inline
Msg::Msg(const char *const &name,
         const char *const &origin,
         const char **const &params,
         const size_t &count):
Msg(name,
    origin? origin : std::string(),
    {params,params+count})
{

}


inline
Msg::Msg(const std::string &name,
         const std::string &origin,
         const Params &params):
code(0),
name(name),
origin(origin),
params(params)
{


}


inline
Msg::Msg(const uint32_t &code,
         const char *const &origin,
         const char **const &params,
         const size_t &count):
Msg(code,
    origin? origin : std::string(),
    {params,params+count})
{

}


inline
Msg::Msg(const uint32_t &code,
         const std::string &origin,
         const Params &params):
code(code),
origin(origin),
params(params)
{


}


inline
std::string Msg::get_nick()
const
{
	char buf[32];
	irc_target_get_nick(get_origin().c_str(),buf,sizeof(buf));
	return buf;
}


inline
std::string Msg::get_host()
const
{
	char buf[128];
	irc_target_get_host(get_origin().c_str(),buf,sizeof(buf));
	return buf;
}

template<class R>
R Msg::get(const size_t &i)
const
{
	return lex_cast<R>(params.at(i));
}


inline
auto &Msg::operator[](const size_t &i)
const try
{
	return get(i);
}
catch(const std::out_of_range &e)
{
	static const std::string empty;
	return empty;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Msg &m)
{
	s << "(" << std::setw(3) << m.get_code() << ")";
	s << " " << std::setw(27) << std::setfill(' ') << std::left << m.get_origin();
	s << " [" << std::setw(2) << m.num_params() << "]: ";

	for(const auto &param : m.get_params())
		s << "[" << param << "]";

	return s;
}
