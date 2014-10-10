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
