/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class NickServ : public Service
{
	Users &users;

	void handle_listchans(const Capture &capture);
	void handle_info(const Capture &capture);
	void captured(const Capture &capture) override;        // Fully collected messages from Service
	NickServ &operator<<(const flush_t f) override;

  public:
	void identify(const std::string &acct, const std::string &pass);
	void regain(const std::string &nick, const std::string &pass = "");
	void ghost(const std::string &nick, const std::string &pass);
	void listchans();

	NickServ(Adb &adb, Sess &sess, Users &users):
	         Service(adb,sess,"NickServ"), users(users) {}
};


inline
void NickServ::listchans()
{
	Stream &out = *this;
	out << "LISTCHANS" << flush;
	next_terminator("channel access match for the nickname");
}


inline
void NickServ::identify(const std::string &acct,
                        const std::string &pass)
{
	Stream &out = *this;
	out << "identify" << " " << acct << " " << pass << flush;
	null_terminator();
}


inline
void NickServ::ghost(const std::string &nick,
                     const std::string &pass)
{
	Stream &out = *this;
	out << "ghost" << " " << nick << " " << pass << flush;
	null_terminator();
}


inline
void NickServ::regain(const std::string &nick,
                      const std::string &pass)
{
	Stream &out = *this;
	out << "regain" << " " << nick << " " << pass << flush;
	null_terminator();
}


inline
void NickServ::captured(const Capture &msg)
{
	const std::string &header = msg.front();

	if(header.find("Information on") == 0)
		handle_info(msg);
	else if(header.find("Access flag(s)") == 0)
		handle_listchans(msg);
	else
		throw Exception("Unhandled NickServ capture.");
}


inline
void NickServ::handle_info(const Capture &msg)
{
	const auto tok = tokens(msg.front());
	const auto &name = tolower(tok.at(2));
	const auto &primary = tolower(tok.at(4).substr(0,tok.at(4).size()-2));  // Chop off "):]"

	Acct acct(get_adb(),&name);
	Adoc info = acct.get("info");
	info.put("account",primary);
	info.put("_fetched_",time(NULL));

	auto it = msg.begin();
	for(++it; it != msg.end(); ++it)
	{
		const auto kv = split(*it," : ");
		const auto &key = chomp(chomp(kv.first),".");
		const auto &val = kv.second;
		info.put(key,val);
	}

	acct.set("info",info);
}


inline
void NickServ::handle_listchans(const Capture &msg)
{
	Sess &sess = get_sess();

	for(const auto &line : msg)
	{
		const auto tok = tokens(line);
		const Mode flags = tok.at(2).substr(1); // chop leading +
		const auto chan = tok.at(4);
		sess.access[chan] = flags;
	}
}


inline
NickServ &NickServ::operator<<(const flush_t)
{
	const scope clear([&]
	{
		clear_sendq();
	});

	Sess &sess = get_sess();
	sess.nickserv << get_str() << flush;
	return *this;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const NickServ &ns)
{
	s << dynamic_cast<const Service &>(ns) << std::endl;
	return s;
}
