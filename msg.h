/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#define SPQF_FMT(type, fields...)  namespace type { enum { fields }; }


inline namespace Fmt
{
SPQF_FMT( NICK,              NICKNAME                                                                        )
SPQF_FMT( QUIT,              REASON                                                                          )
SPQF_FMT( JOIN,              CHANNAME                                                                        )
SPQF_FMT( PART,              CHANNAME, REASON                                                                )
SPQF_FMT( MODE,              CHANNAME, DELTASTR                                                              )
SPQF_FMT( UMODE,             DELTASTR                                                                        )
SPQF_FMT( UMODEIS,           NICKNAME, DELTASTR                                                              )
SPQF_FMT( CHANNELMODEIS,     NICKNAME, CHANNAME, DELTASTR                                                    )
SPQF_FMT( KICK,              CHANNAME, TARGET,   REASON                                                      )
SPQF_FMT( CHANMSG,           CHANNAME, TEXT                                                                  )
SPQF_FMT( CNOTICE,           CHANNAME, TEXT                                                                  )
SPQF_FMT( PRIVMSG,           NICKNAME, TEXT                                                                  )
SPQF_FMT( NOTICE,            NICKNAME, TEXT                                                                  )
SPQF_FMT( NAMREPLY,          NICKNAME, TYPE,     CHANNAME, NAMELIST                                          )
SPQF_FMT( WHOREPLY,          SELFNAME, CHANNAME, USERNAME, HOSTNAME, SERVNAME, NICKNAME, FLAGS,    ADDL      )
SPQF_FMT( WHOISIDLE,         SELFNAME, NICKNAME, SECONDS,  REMARKS                                           )
SPQF_FMT( WHOISSECURE,       SELFNAME, NICKNAME, REMARKS,                                                    )
SPQF_FMT( WHOISACCOUNT,      SELFNAME, NICKNAME, ACCOUNT,  REMARKS                                           )
SPQF_FMT( CREATIONTIME,      SELFNAME, CHANNAME, TIME,                                                       )
SPQF_FMT( BANLIST,           SELFNAME, CHANNAME, BANMASK,  OPERATOR, TIME,                                   )
SPQF_FMT( QUIETLIST,         SELFNAME, CHANNAME, BANMASK,  OPERATOR, TIME,                                   )
}


class Msg
{

	using Params = std::vector<std::string>;

	uint32_t code;
	std::string origin;
	Params params;

  public:
	const uint32_t &get_code() const                        { return code;                      }
	const std::string &get_origin() const                   { return origin;                    }
	const Params &get_params() const                        { return params;                    }
	size_t num_params() const                               { return get_params().size();       }

	const std::string &get(const size_t &i) const           { return get_params().at(i);        }
	const std::string &operator[](const size_t &i) const;   // returns empty str for outofrange
	template<class R> R get(const size_t &i) const;         // throws for range or bad cast
	template<class R> R operator[](const size_t &i) const;  // throws for range or bad cast

	std::string get_nick() const;
	std::string get_host() const;
	bool from_server() const                                { return get_nick() == get_host();  }

	Msg(const uint32_t &code, const std::string &origin, const Params &params);
	Msg(const uint32_t &code, const char *const &origin, const char **const &params, const size_t &count);

	friend std::ostream &operator<<(std::ostream &s, const Msg &m);
};


inline
Msg::Msg(const uint32_t &code,
         const char *const &origin,
         const char **const &params,
         const size_t &count):
code(code),
origin(origin),
params(params,params+count)
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
R Msg::operator[](const size_t &i)
const
{
	return get<R>(i);
}


template<class R>
R Msg::get(const size_t &i)
const
{
	const std::string &str = params.at(i);
	return boost::lexical_cast<R>(str);
}


inline
const std::string &Msg::operator[](const size_t &i)
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
	s << " (" << std::setw(2) << m.num_params() << "): ";

	for(const auto &param : m.get_params())
		s << "[" << param << "]";

	return s;
}
