/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class ChanServ : public Service
{
	void handle_flags(const Capture &capture);
	void handle_info(const Capture &capture);
	void captured(const Capture &capture) override;

  public:
	void query_access_list(const std::string &name);
	void query_flags(const std::string &name);
	void query_info(const std::string &name);

	ChanServ &operator<<(const flush_t f) override;

	ChanServ(Adb &adb, Sess &sess):
	         Service(adb,sess,"ChanServ") {}
};


inline
void ChanServ::query_info(const std::string &acct)
{
	Locutor &out = *this;
	out << "info " << acct << flush;
	next_terminator("*** End of Info ***");
}


inline
void ChanServ::query_flags(const std::string &acct)
{
	Locutor &out = *this;
	out << "flags " << acct << flush;

	std::stringstream ss;
	ss << "End of " << acct << " FLAGS listing.";
	next_terminator(ss.str());
}


inline
void ChanServ::query_access_list(const std::string &acct)
{
	Locutor &out = *this;
	out << "access " << acct << " list" << flush;

	std::stringstream ss;
	ss << "End of " << acct << " FLAGS listing.";
	next_terminator(ss.str());
}


inline
void ChanServ::captured(const Capture &msg)
{
	const std::string &header = msg.front();

	if(header.find("Information on") == 0)
		handle_info(msg);
	else if(header.find("Entry") == 0)
		handle_flags(msg);
	else
		throw Exception("Unhandled ChanServ capture.");
}


inline
void ChanServ::handle_info(const Capture &msg)
{
	const std::vector<std::string> tok = tokens(msg.front());
	const std::string name = tolower(chomp(tok.at(2),":"));

	Acct acct(get_adb(),name);
	Adoc info = acct.get("info");

	auto it = msg.begin();
	for(++it; it != msg.end(); ++it)
	{
		const auto kv = split(*it," : ");
		const std::string &key = chomp(chomp(kv.first),".");
		const std::string &val = kv.second;
		info.put(key,val);
	}

	acct.set("info",info);
}


inline
void ChanServ::handle_flags(const Capture &msg)
{
	const std::string name = tolower(tokens(get_terminator()).at(2));

	Acct acct(get_adb(),name);
	Adoc flags = acct.get("flags");

	auto it = msg.begin();
	auto end = msg.begin();
	std::advance(it,2);
	std::advance(end,msg.size() - 1);
	for(; it != end; ++it)
	{
		const std::vector<std::string> toks = tokens(*it," ");
		const std::string &num = toks.at(0);
		const std::string &user = toks.at(1);
		const std::string &list = toks.at(2);
		std::stringstream addl;
		for(size_t i = 3; i < toks.size(); i++)
			addl << toks.at(i) << (i == toks.size() - 1? "" : " ");

		Adoc ent;
		ent.put("user",user);
		ent.put("flag",list);
		ent.put("addl",addl.str());
		flags.push_back(std::make_pair("",ent));
	}

	acct.set("flags",flags);
}


inline
ChanServ &ChanServ::operator<<(const flush_t f)
{
	Sess &sess = get_sess();
	auto &sendq = get_sendq();
	sess.chanserv(sendq.str());
	clear_sendq();
	return *this;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const ChanServ &cs)
{
	s << dynamic_cast<const Service &>(cs) << std::endl;
	return s;
}
