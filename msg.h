/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


inline namespace Fmts
{
	namespace NICK            { enum { NICKNAME                                                                            }; }
	namespace QUIT            { enum { REASON                                                                              }; }
	namespace JOIN            { enum { CHANNAME                                                                            }; }
	namespace PART            { enum { CHANNAME,  REASON                                                                   }; }
	namespace MODE            { enum { CHANNAME,  DELTASTR                                                                 }; }
	namespace UMODE           { enum { DELTASTR                                                                            }; }
	namespace UMODEIS         { enum { NICKNAME,  DELTASTR                                                                 }; }
	namespace CHANNELMODEIS   { enum { NICKNAME,  CHANNAME,  DELTASTR                                                      }; }
	namespace KICK            { enum { CHANNAME,  TARGET,    REASON                                                        }; }
	namespace CHANMSG         { enum { CHANNAME,  TEXT                                                                     }; }
	namespace CNOTICE         { enum { CHANNAME,  TEXT                                                                     }; }
	namespace PRIVMSG         { enum { NICKNAME,  TEXT                                                                     }; }
	namespace NOTICE          { enum { NICKNAME,  TEXT                                                                     }; }
	namespace NAMREPLY        { enum { NICKNAME,  TYPE,      CHANNAME,   NAMELIST                                          }; }
	namespace WHOREPLY        { enum { SELFNAME,  CHANNAME,  USERNAME,   HOSTNAME,  SERVNAME,  NICKNAME,  FLAGS,   ADDL    }; }
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

	const std::string &get_param(const size_t &i) const     { return get_params().at(i);        }
	const std::string &operator[](const size_t &i) const;   // returns empty str for outofrange
	size_t num_params() const                               { return get_params().size();       }

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


inline
const std::string &Msg::operator[](const size_t &i)
const try
{
	return get_param(i);
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
