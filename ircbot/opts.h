/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Opts : public std::map<std::string,std::string>
{
	Opts(): std::map<std::string,std::string>
	{
		// Client info
		{"nick",                ""                                        },
		{"user",                "SPQF"                                    },
		{"gecos",               "Senate & People of Freenode (#SPQF)"     },

		// Nickserv identification
		{"ns-acct",             ""                                        },
		{"ns-pass",             ""                                        },

		// Server info
		{"host",                "chat.freenode.net"                       },
		{"port",                "6667"                                    },
		{"pass",                ""                                        },

		// Misc configuration
		{"locale",              ""                                        },
		{"dbdir",               "db"                                      },
		{"logdir",              "logs"                                    },
		{"prefix",              "!"                                       },
		{"invite",              ""                                        },
		{"invite-throttle",     "300"                                     },
		{"owner",               ""                                        },
		{"services",            "true"                                    },
	}
	{
		if(at("locale").empty())
		{
			const char *const loc = secure_getenv("LANG");
			if(loc)
				at("locale") = loc;
		}
	}

	// Channels to join on connect
	std::list<std::string> autojoin;

	// Direct access to the config
	const std::string &operator[](const std::string &key) const;
	std::string &operator[](const std::string &key);

	template<class T> using non_num_t = typename std::enable_if<!std::is_arithmetic<T>(),T>::type;
	template<class T> using num_t = typename std::enable_if<std::is_arithmetic<T>::value,T>::type;

	template<class T> non_num_t<T> get(const std::string &key) const;
	template<class T> num_t<T> get(const std::string &key) const;

	bool parse_arg(const std::string &arg);
	uint parse(const std::vector<std::string> &argv);

	// Output this info
	friend std::ostream &operator<<(std::ostream &s, const Opts &id);
};


inline
uint Opts::parse(const std::vector<std::string> &strs)
{
	uint ret = 0;
	for(const auto &str : strs)
		ret += parse_arg(str);

	return ret;
}


inline
bool Opts::parse_arg(const std::string &str)
{
	if(str.find("--") != 0)
		return false;

	const size_t eqp = str.find('=');
	if(eqp == str.npos)
	{
		const std::string key = str.substr(2);
		(*this)[key] = "true";
		return true;
	}

	const std::string key = str.substr(2,eqp-2);
	const std::string val = str.substr(eqp+1);

	if(key == "join")
		autojoin.emplace_back(val);
	else
		(*this)[key] = val;

	return true;
}


template<> inline
bool Opts::get<bool>(const std::string &key)
const try
{
	const auto it = this->find(key);
	if(it == end())
		return false;

	const auto &val = it->second;
	switch(hash(tolower(val)))
	{
		case hash("enable"):
		case hash("true"):
		case hash("one"):
		case hash("1"):
			return true;

		default:
			return false;
	}
}
catch(const boost::bad_lexical_cast &e)
{
	std::cerr << "Bad boolean value for configuration key: "
	          << "[" << key << "] "
	          << "(" << e.what() << ")"
	          << std::endl;

	return false;
}


template<class T>
Opts::non_num_t<T> Opts::get(const std::string &key)
const
{
	return lex_cast<T>(this->at(key));
}


template<class T>
Opts::num_t<T> Opts::get(const std::string &key)
const try
{
	return lex_cast<T>(this->at(key));
}
catch(const boost::bad_lexical_cast &e)
{
	std::cerr << "Bad numerical value for configuration key: "
	          << "[" << key << "] "
	          << "(" << e.what() << ")"
	          << std::endl;

	return T(0);
}
catch(const std::out_of_range &e)
{
	return T(0);
}


inline
std::string &Opts::operator[](const std::string &key)
{
	const auto it = find(key);
	return it == end()? emplace(key,std::string()).first->second:
	                    it->second;
}


inline
const std::string &Opts::operator[](const std::string &key)
const
{
	const auto it = find(key);
	if(it == end())
		throw std::out_of_range("Key not found");

	return it->second;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Opts &id)
{
	for(const auto &pair : id)
		std::cout << std::setw(16) << std::left << pair.first
		          << " => "
		          << pair.second
		          << std::endl;

	std::cout << std::endl;
	std::cout << "Autojoin channels: " << std::endl;
	for(const auto &chan : id.autojoin)
		std::cout << "\t" << chan << std::endl;

	return s;
}
