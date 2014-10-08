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
		const char *type() const    { return "config";   }

		std::string key;
		std::string val;

		void passed();
		void starting();

	  public:
		template<class... Args> Config(Args&&... args);
	};

	class Mode : public Vote
	{
		const char *type() const    { return "mode";     }

		void passed();
		void starting();

	  public:
		template<class... Args> Mode(Args&&... args): Vote(std::forward<Args>(args)...) {}
	};

	class Kick : public Vote
	{
		const char *type() const    { return "kick";     }

		void starting();
		void passed();

	  public:
		template<class... Args> Kick(Args&&... args): Vote(std::forward<Args>(args)...) {}
	};

	class Invite : public Vote
	{
		const char *type() const    { return "invite";   }

		void passed();

	  public:
		template<class... Args> Invite(Args&&... args): Vote(std::forward<Args>(args)...) {}
	};

	class Topic : public Vote
	{
		const char *type() const    { return "topic";    }

		void passed();

	  public:
		template<class... Args> Topic(Args&&... args): Vote(std::forward<Args>(args)...) {}
	};

	class Opine : public Vote
	{
		const char *type() const    { return "opine";    }

		void passed();

	  public:
		template<class... Args> Opine(Args&&... args): Vote(std::forward<Args>(args)...) {}
	};

	class Ban : public Vote
	{
		const char *type() const    { return "ban";      }

		void starting();
		void passed();

	  public:
		template<class... Args> Ban(Args&&... args): Vote(std::forward<Args>(args)...) {}
	};

	class Quiet : public Vote
	{
		const char *type() const    { return "quiet";    }

		void starting();
		void passed();

	  public:
		template<class... Args> Quiet(Args&&... args): Vote(std::forward<Args>(args)...) {}
	};

	class UnQuiet : public Vote
	{
		const char *type() const    { return "unquiet";  }

		void starting();
		void passed();

	  public:
		template<class... Args> UnQuiet(Args&&... args): Vote(std::forward<Args>(args)...) {}
	};
}


inline
void vote::UnQuiet::starting()
{
	const std::string &victim = get_issue();
	User &user = get_users().get(victim);
	user.whois();
}


inline
void vote::UnQuiet::passed()
{
	Chan &chan = get_chan();
	const Adoc &cfg = get_cfg();
	const User &user = get_users().get(get_issue());
	chan.unquiet(user);
}



inline
void vote::Quiet::starting()
{
	const std::string &victim = get_issue();
	User &user = get_users().get(victim);

	if(user.is_myself())
		throw Exception("http://en.wikipedia.org/wiki/Areopagitica");

	user.whois();
}


inline
void vote::Quiet::passed()
{
	Chan &chan = get_chan();
	const Adoc &cfg = get_cfg();
	const User &user = get_users().get(get_issue());
	chan.quiet(user);
}



inline
void vote::Ban::starting()
{
	const std::string &victim = get_issue();
	User &user = get_users().get(victim);

	if(user.is_myself())
		throw Exception("http://en.wikipedia.org/wiki/Leviathan_(book)");

	user.whois();
}


inline
void vote::Ban::passed()
{
	Chan &chan = get_chan();
	const Adoc &cfg = get_cfg();
	const User &user = get_users().get(get_issue());
	chan.ban(user);
	chan.remove(user,"And I ain't even mad");
}



inline
void vote::Opine::passed()
{
	using namespace colors;

	Chan &chan = get_chan();
	chan << "The People of " << chan.get_name() << " decided "
	     << BOLD << get_issue() << OFF
	     << flush;
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

	if(cfg["kick.ignore_away"] == "1" && user.is_away())
		throw Exception("The user is currently away and config.vote.kick.ignore_away == 1");

	chan.kick(user,"Voted off the island");
}



inline
void vote::Invite::passed()
{
	Chan &chan = get_chan();
	chan << "An invite to " << get_issue() << " is being sent..." << flush;
	chan.invite(get_issue());
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



inline
void vote::Topic::passed()
{
	Chan &chan = get_chan();
	chan.topic(get_issue());
}



template<class... Args>
vote::Config::Config(Args&&... args):
Vote(std::forward<Args>(args)...)
{
	using delim = boost::char_separator<char>;

	static const delim sep("=");
	const boost::tokenizer<delim> exprs(get_issue(),sep);
	std::vector<std::string> tokens(exprs.begin(),exprs.end());

	if(tokens.size() > 8)
		throw Exception("Path nesting too deep.");

	if(tokens.size() > 0)
	{
		const auto it = std::remove(tokens.at(0).begin(),tokens.at(0).end(),' ');
		tokens.at(0).erase(it,tokens.at(0).end());
		key = tokens.at(0);
	}
	else throw Exception("Invalid syntax to assign a configuration variable.");

	if(tokens.size() > 1)
	{
		const auto it = std::remove(tokens.at(1).begin(),tokens.at(1).end(),' ');
		tokens.at(1).erase(it,tokens.at(1).end());
		val = tokens.at(1);
	}
}


inline
void vote::Config::starting()
{
	using namespace colors;

	Chan &chan = get_chan();
	const Adoc &cfg = chan.get();

	if(!val.empty() && cfg[key] == val)
		throw Exception("Variable already set to that value.");

	if(val.empty())
		chan << "Note: vote deletes variable [" << BOLD << key << OFF << "] "
		     << BOLD << "and all child variables" << OFF << "."
		     << flush;

	if(!val.empty()) try
	{
		// Only use numerical values for now, throw otherwise
		boost::lexical_cast<size_t>(val);
	}
	catch(const boost::bad_lexical_cast &e)
	{
		throw Exception("Must use a numerical value for this key.");
	}

	chan << "Note: "
	     << UNDER2 << "Changing the configuration can be " << BOLD << "DANGEROUS" << OFF << ", "
	     << "potentially breaking the ability for future votes to revert this change."
	     << flush;
}


inline
void vote::Config::passed()
{
	Chan &chan = get_chan();
	Adoc cfg = chan.get();

	if(!val.empty())
		cfg.put(key,val);
	else if(!cfg.remove(key))
		throw Exception("The configuration key was not found.");

	chan.set(cfg);
}
