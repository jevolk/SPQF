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
		{"nickname",   "SPQF"                         },
		{"username",   "SPQF"                         },
		{"fullname",   "Senate & People of Freenode"  },

		// Server info
		{"hostname",   "chat.freenode.net"            },
		{"port",       "6667"                         },
		{"pass",       ""                             },

		// Misc configuration
		{"dbdir",      "db"                           },
	}{}

	// Channels to join on connect
	std::list<std::string> autojoin;

	// Direct access to the config
	const std::string &operator[](const std::string &key) const  { return this->at(key); }
	std::string &operator[](const std::string &key)              { return this->at(key); }

	// Output this info
	friend std::ostream &operator<<(std::ostream &s, const Ident &id);
};


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
