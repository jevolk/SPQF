/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class NickServ : public Service
{
	Users &users;

    NickServ &operator<<(const flush_t f) override;

	void handle_info(const Capture &capture);
	void captured(const Capture &capture) override;        // Fully collected messages from Service

  public:
	void query_info(const std::string &acct);

	NickServ(Adb &adb, Sess &sess, Users &users);
};


inline
NickServ::NickServ(Adb &adb,
                   Sess &sess,
                   Users &users):
Service(adb,sess,"NickServ"),
users(users)
{


}


inline
void NickServ::query_info(const std::string &acct)
{
	Locutor &out = *this;
	out << "info " << acct << flush;
	next_terminator("*** End of Info ***");
}


inline
void NickServ::captured(const Capture &msg)
{
	const std::string &header = msg.front();

	if(header.find("Information on") == 0)
		handle_info(msg);
	else
		throw Exception("Unhandled NickServ capture.");
}


inline
void NickServ::handle_info(const Capture &msg)
{
	const std::vector<std::string> tok = tokens(msg.front());
	const auto &name = tolower(tok.at(2));
	const auto &primary = tolower(tok.at(4).substr(0,tok.at(4).size()-2));  // Chop off "):]"

	Acct acct(get_adb(),name);
	Adoc info = acct.get("info");
	info.put("account",primary);

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
NickServ &NickServ::operator<<(const flush_t f)
{
	Sess &sess = get_sess();
	auto &sendq = get_sendq();
	sess.nickserv(sendq.str());
	clear_sendq();
	return *this;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const NickServ &ns)
{
	s << dynamic_cast<const Service &>(ns) << std::endl;
	return s;
}
