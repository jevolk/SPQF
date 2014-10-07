/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


namespace vote
{
	class Config : public Vote
	{
		std::string key;
		std::string val;

		void passed();
		void starting();

	  public:
		template<class... Args> Config(Args&&... args);
	};

	class Mode : public Vote
	{
		void passed();
		void starting();

	  public:
		template<class... Args> Mode(Args&&... args);
	};

	class Kick : public Vote
	{
		void starting();
		void passed();

	  public:
		template<class... Args> Kick(Args&&... args);
	};

	class Invite : public Vote
	{
		void passed();

	  public:
		template<class... Args> Invite(Args&&... args);
	};
}


template<class... Args>
vote::Kick::Kick(Args&&... args):
Vote(std::forward<Args>(args)...)
{

}


inline
void vote::Kick::starting()
{
	const std::string &victim = get_issue();
	User &user = get_users().get(victim);

	if(user.is_myself())
		throw Exception("GNU philosophy 101: You're not free to be unfree.");

	user.whois();
}


inline
void vote::Kick::passed()
{
	Chan &chan = get_chan();
	const Adoc &cfg = get_cfg();
	const User &user = get_users().get(get_issue());

	if(cfg["kick.if_away"] == "0" && user.is_away())
		throw Exception("The user is currently away and config.vote.kick.if_away == 0");

	chan.kick(user,"Voted off the island");
}



template<class... Args>
vote::Invite::Invite(Args&&... args):
Vote(std::forward<Args>(args)...)
{

}


inline
void vote::Invite::passed()
{
	Chan &chan = get_chan();
	chan << "An invite to " << get_issue() << " is being sent..." << flush;
	chan.invite(get_issue());
}



template<class... Args>
vote::Mode::Mode(Args&&... args):
Vote(std::forward<Args>(args)...)
{

}


inline
void vote::Mode::passed()
{
	Chan &chan = get_chan();
	chan.mode(get_issue());
}


inline
void vote::Mode::starting()
{
	const Sess &sess = get_sess();
	const Server &serv = sess.get_server();
	const Deltas deltas(get_issue(),serv);

	Chan &chan = get_chan();
}



template<class... Args>
vote::Config::Config(Args&&... args):
Vote(std::forward<Args>(args)...)
{
	using delim = boost::char_separator<char>;

	static const delim sep("=");
	const boost::tokenizer<delim> exprs(get_issue(),sep);
	std::vector<std::string> tokens(exprs.begin(),exprs.end());
	if(tokens.size() != 2 || tokens.at(0).empty() || tokens.at(1).empty())
		throw Exception("Invalid syntax to assign a configuration variable.");

	key = tokens.at(0);
	if(key.back() == ' ')
		key.pop_back();

	val = tokens.at(1);
	if(val.front() == ' ')
		val.erase(val.begin());
}


inline
void vote::Config::starting()
{
	const Chan &chan = get_chan();
	const Adoc &cfg = chan.get();

	if(!cfg.has(key))
		throw Exception("Variable not found in channel's configuration.");

	if(cfg[key] == val)
		throw Exception("Variable already set to that value.");
}


inline
void vote::Config::passed()
{
	Chan &chan = get_chan();
	Adoc cfg = chan.get();
	cfg.put(key,val);
	chan.set(cfg);
}
