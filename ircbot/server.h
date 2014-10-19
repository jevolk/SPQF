/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct ISupport : std::map<std::string,std::string>
{
	bool empty(const std::string &key) const         { return !count(key) || at(key).empty();         }
	bool operator()(const std::string &key) const    { return count(key);                             }

	const std::string &operator[](const std::string &key) const;                        // empty static string

	template<class T = std::string> T get(const std::string &key) const;                // throws
	template<class T = std::string> T get(const std::string &key, T&& def_val) const;   // defaults

	template<class T = size_t> T get_or_max(const std::string &key) const;              // numeric lim
	template<class... A> bool find_in(const std::string &key, A&&... val) const;

	template<class... A> ISupport(A&&... a): std::map<std::string,std::string>(std::forward<A>(a)...) {}
};


struct Server
{
	std::string name;
	std::string vers;
	ISupport isupport;
	std::string user_modes, user_pmodes;
	std::string chan_modes, chan_pmodes;
	std::string serv_modes, serv_pmodes;
	std::set<std::string> caps;

	bool has_cap(const std::string &cap) const       { return caps.count(cap);                        }

	// 3.2 CHANLIMIT
	size_t get_chanlimit(const char &prefix) const;

	// 3.14 PREFIX
	bool has_prefix(const char &prefix) const;
	char prefix_to_mode(const char &prefix) const;
	char mode_to_prefix(const char &mode) const;

	friend std::ostream &operator<<(std::ostream &s, const Server &srv);
};


inline
bool Server::has_prefix(const char &prefix)
const try
{
	const std::string &pxs = isupport["PREFIX"];
	const std::string pfx = split(pxs,")").second;
	return pfx.find(prefix) != std::string::npos;
}
catch(const std::out_of_range &e)
{
	throw Exception("PREFIX was not stocked by an ISUPPORT message.");
}


inline
char Server::mode_to_prefix(const char &prefix)
const try
{
	const std::string &pxs = isupport["PREFIX"];
	const std::string modes = between(pxs,"(",")");
	const std::string prefx = split(pxs,")").second;
	return prefx.at(modes.find(prefix));
}
catch(const std::out_of_range &e)
{
	throw Exception("No prefix for that mode on this server.");
}


inline
char Server::prefix_to_mode(const char &mode)
const try
{
	const std::string &pxs = isupport["PREFIX"];
	const std::string modes = between(pxs,"(",")");
	const std::string prefx = split(pxs,")").second;
	return modes.at(prefx.find(mode));
}
catch(const std::out_of_range &e)
{
	throw Exception("No mode for that prefix on this server.");
}


inline
size_t Server::get_chanlimit(const char &prefix)
const
{
	for(const auto &tok : tokens(isupport["CHANLIMIT"],","))
	{
		const auto kv = split(tok,":");
		if(kv.first.find(prefix) != std::string::npos)
			return kv.second.empty()? std::numeric_limits<size_t>::max():
			                          lex_cast<size_t>(kv.second);
	}

	return std::numeric_limits<size_t>::max();
}



template<class... A>
bool ISupport::find_in(const std::string &key,
                       A&&... val)
const
{
	const auto it = find(key);
	if(it == end())
		return false;

	return it->second.find(std::forward<A>(val)...) != std::string::npos;
}


template<class T>
T ISupport::get_or_max(const std::string &key)
const
{
	return get(key,std::numeric_limits<T>::max());
}


template<class T>
T ISupport::get(const std::string &key,
                T&& def_val)
const
{
	return empty(key)? def_val : get<T>(key);
}


template<class T>
T ISupport::get(const std::string &key)
const
{
	return lex_cast<T>(at(key));
}


inline
const std::string &ISupport::operator[](const std::string &key)
const try
{
	return at(key);
}
catch(const std::out_of_range &e)
{
	static const std::string mt;
	return mt;
}



inline
std::ostream &operator<<(std::ostream &s,
                         const Server &srv)
{
	s << "[Server Modes]: " << std::endl;
	s << "name:           " << srv.name << std::endl;
	s << "vers:           " << srv.vers << std::endl;
	s << "user modes:     [" << srv.user_modes << "]" << std::endl;
	s << "   w/ parm:     [" << srv.user_pmodes << "]" << std::endl;
	s << "chan modes:     [" << srv.chan_modes << "]" << std::endl;
	s << "   w/ parm:     [" << srv.chan_pmodes << "]" << std::endl;
	s << "serv modes:     [" << srv.serv_modes << "]" << std::endl;
	s << "   w/ parm:     [" << srv.serv_pmodes << "]" << std::endl;
	s << std::endl;

	s << "[Server Configuration]:" << std::endl;
	for(const auto &p : srv.isupport)
		s << std::setw(16) << p.first << " = " << p.second << std::endl;

	s << std::endl;

	s << "[Server Extended Capabilities]:" << std::endl;
	for(const auto &cap : srv.caps)
		s << cap << std::endl;

	return s;
}
