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

	std::string user_modes, user_pmodes;
	std::string chan_modes, chan_pmodes;
	std::string serv_modes, serv_pmodes;

	std::map<std::string,std::string> cfg;
	std::set<std::string> caps;

	bool has_cap(const std::string &cap) const     { return caps.count(cap);  }
	char prefix_to_mode(const char &prefix) const;
	char mode_to_prefix(const char &mode) const;
	bool has_prefix(const char &prefix) const;

	friend std::ostream &operator<<(std::ostream &s, const Server &srv);
};


inline
bool Server::has_prefix(const char &prefix)
const try
{
	const std::string &pxs = cfg.at("PREFIX");
	const std::string prefx = split(pxs,")").second;
	return prefx.find(prefix) != std::string::npos;
}
catch(const std::out_of_range &e)
{
	throw Exception("PREFIX was not stocked by an ISUPPORT message.");
}


inline
char Server::mode_to_prefix(const char &prefix)
const try
{
	const std::string &pxs = cfg.at("PREFIX");
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
	const std::string &pxs = cfg.at("PREFIX");
	const std::string modes = between(pxs,"(",")");
	const std::string prefx = split(pxs,")").second;
	return modes.at(prefx.find(mode));
}
catch(const std::out_of_range &e)
{
	throw Exception("No mode for that prefix on this server.");
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
	for(const auto &p : srv.cfg)
		s << std::setw(16) << p.first << " = " << p.second << std::endl;

	s << std::endl;

	s << "[Server Extended Capabilities]:" << std::endl;
	for(const auto &cap : srv.caps)
		s << cap << std::endl;

	return s;
}
