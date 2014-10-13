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

	friend std::ostream &operator<<(std::ostream &s, const Server &srv);
};


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
