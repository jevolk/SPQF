/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


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

	// 3.10 MAXLIST
	std::string get_maxlist_assoc(const char &prefix) const;
	size_t get_maxlist(const char &prefix) const;

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
size_t Server::get_maxlist(const char &prefix)
const
{
	for(const auto &tok : tokens(isupport["MAXLIST"],","))
	{
		const auto kv = split(tok,":");
		if(kv.first.find(prefix) != std::string::npos)
			return kv.second.empty()? std::numeric_limits<size_t>::max():
			                          lex_cast<size_t>(kv.second);
	}

	return std::numeric_limits<size_t>::max();
}


inline
std::string Server::get_maxlist_assoc(const char &prefix)
const
{
	for(const auto &tok : tokens(isupport["MAXLIST"],","))
	{
		const auto kv = split(tok,":");
		if(kv.first.find(prefix) != std::string::npos)
			return kv.first;
	}

	return {};
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
