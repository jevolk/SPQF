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
		template<class... Args> Mode(Args&&... args): Vote("mode",std::forward<Args>(args)...) {}
	};

	class Kick : public Vote
	{
		void starting();
		void passed();

	  public:
		template<class... Args> Kick(Args&&... args): Vote("kick",std::forward<Args>(args)...) {}
	};

	class Invite : public Vote
	{
		void passed();

	  public:
		template<class... Args> Invite(Args&&... args): Vote("invite",std::forward<Args>(args)...) {}
	};

	class Topic : public Vote
	{
		void passed();

	  public:
		template<class... Args> Topic(Args&&... args): Vote("topic",std::forward<Args>(args)...) {}
	};

	class Opine : public Vote
	{
		void passed();

	  public:
		template<class... Args> Opine(Args&&... args): Vote("opine",std::forward<Args>(args)...) {}
	};

	class Ban : public Vote
	{
		void starting();
		void passed();

	  public:
		template<class... Args> Ban(Args&&... args): Vote("ban",std::forward<Args>(args)...) {}
	};

	class Quiet : public Vote
	{
		void starting();
		void passed();

	  public:
		template<class... Args> Quiet(Args&&... args): Vote("quiet",std::forward<Args>(args)...) {}
	};

	class UnQuiet : public Vote
	{
		void starting();
		void passed();

	  public:
		template<class... Args> UnQuiet(Args&&... args): Vote("unquiet",std::forward<Args>(args)...) {}
	};
}


template<class... Args>
vote::Config::Config(Args&&... args):
Vote("config",std::forward<Args>(args)...)
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
