/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class ChanServ : public Service
{
	Chans &chans;

	void handle_akicklist(const Capture &capture);
	void handle_flagslist(const Capture &capture);
	void handle_info(const Capture &capture);
	void captured(const Capture &capture) override;
	ChanServ &operator<<(const flush_t f) override;

	void handle_set_flags(Chan &chan, const std::string &flags, const Mask &targ);

  public:
	void handle_cnotice(const Msg &msg, Chan &chan);
	void handle(const Msg &msg);

	ChanServ(Adb &adb, Sess &sess, Chans &chans):
	         Service(adb,sess,"ChanServ"), chans(chans) {}
};


inline
void ChanServ::handle(const Msg &msg)
{
	using namespace fmt::NOTICE;

	// ignore entry message
	if(!msg[TEXT].empty() && msg[TEXT].at(0) == '[')
		return;

	Service::handle(msg);
}


inline
void ChanServ::captured(const Capture &msg)
{
	const auto &header = msg.front();
	if(header.find("Information on") == 0)
		handle_info(msg);
	else if(header.find("Entry") == 0)
		handle_flagslist(msg);
	else if(header.find("AKICK") == 0)
		handle_akicklist(msg);
	else
		throw Exception("Unhandled ChanServ capture.");
}


inline
void ChanServ::handle_cnotice(const Msg &msg,
                              Chan &chan)
{
	using namespace fmt::CNOTICE;

	const auto toks = tokens(decolor(msg[TEXT]));
	if(toks.size() == 6 && toks.at(1) == "set" && toks.at(2) == "flags")
		handle_set_flags(chan,toks.at(3),chomp(toks.at(5),"."));
}


inline
void ChanServ::handle_set_flags(Chan &chan,
                                const std::string &flags,
                                const Mask &targ)
{
	chan.lists.delta_flag(targ,flags);
}


inline
void ChanServ::handle_info(const Capture &msg)
{
	const std::vector<std::string> tok = tokens(msg.front());
	const std::string name = tolower(chomp(tok.at(2),":"));

	Chan &chan = chans.get(name);

	chan::Info info;
	auto it = msg.begin();
	for(++it; it != msg.end(); ++it)
	{
		const auto kv = split(*it," : ");
		const std::string &key = chomp(chomp(kv.first),".");
		const std::string &val = kv.second;
		info.emplace(key,val);
	}

	chan.set_info(info);
}


inline
void ChanServ::handle_flagslist(const Capture &msg)
{
	const std::string name = tolower(tokens(get_terminator().front()).at(2));

	Chan &chan = chans.get(name);

	decltype(chan.lists.flags) flags;
	auto it = msg.begin();
	auto end = msg.begin();
	std::advance(it,2);
	std::advance(end,msg.size() - 1);
	for(; it != end; ++it)
	{
		const auto toks = tokens(*it," ");
		const auto &num = toks.at(0);
		const auto &user = tolower(toks.at(1));
		const auto &list = toks.at(2).substr(1);   // chop off '+'
		const bool founder = toks.at(3) == "(FOUNDER)";

		std::stringstream addl;
		for(size_t i = 3; i < toks.size(); i++)
			addl << toks.at(i) << (i == toks.size() - 1? "" : " ");

		flags.emplace(user,list,0,founder);   //TODO: times
	}

	chan.lists.flags = flags;
}


inline
void ChanServ::handle_akicklist(const Capture &msg)
{
	auto it = msg.begin();
	const size_t ns = tokens(*it).at(3).size() - 1;
	const std::string name = tolower(tokens(*it).at(3).substr(0,ns));
	++it;

	Chan &chan = chans.get(name);
	decltype(chan.lists.akicks) akicks;

	auto end = msg.begin();
	std::advance(end,msg.size());
	for(; it != end; ++it)
	{
		const std::string &str = *it;
		const std::vector<std::string> toks = tokens(str," ");
		const std::string &num = toks.at(0);
		const std::string &mask = toks.at(1);
		const std::pair<std::string,std::string> reason = split(between(str,"(",")")," | ");
		const std::string details = between(str,"[","]");
		const std::string setter = between(details,"setter: ",",");
		const std::string expires = between(details,"expires: ",",");    //TODO: 
		const std::string modified = between(details,"modified: ",",");  //TODO: 

		akicks.emplace(mask,setter,reason.first,reason.second,0,0); //TODO: times
	}

	chan.lists.akicks = akicks;
}


inline
ChanServ &ChanServ::operator<<(const flush_t)
{
	const scope clear([&]
	{
		Stream::clear();
	});

	Quote cs(get_sess(),"cs");
	cs << get_str() << flush;
	return *this;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const ChanServ &cs)
{
	s << dynamic_cast<const Service &>(cs) << std::endl;
	return s;
}
