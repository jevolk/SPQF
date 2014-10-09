/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Ident : public std::map<std::string,std::string>
{
	Ident(): std::map<std::string,std::string>
	{
		// Client info
		{"nick",       "SPQF"                         },
		{"user",       "SPQF"                         },
		{"gecos",      "Senate & People of Freenode"  },

		// Nickserv identification
		{"ns-acct",    ""                             },
		{"ns-pass",    ""                             },

		// Server info
		{"host",       ""                             },
		{"port",       "6667"                         },
		{"pass",       ""                             },

		// Misc configuration
		{"dbdir",      "db"                           },
		{"logdir",     "logs"                         },
	}{}

	// Channels to join on connect
	std::list<std::string> autojoin;

	// Direct access to the config
	const std::string &operator[](const std::string &key) const;
	std::string &operator[](const std::string &key);

	// Output this info
	friend std::ostream &operator<<(std::ostream &s, const Ident &id);
};


inline
std::string &Ident::operator[](const std::string &key)
{
	const auto it = find(key);
	return it == end()? emplace(key,std::string()).first->second:
	                    it->second;
}


inline
const std::string &Ident::operator[](const std::string &key)
const
{
	const auto it = find(key);
	if(it == end())
		throw std::out_of_range("Key not found");

	return it->second;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Ident &id)
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
