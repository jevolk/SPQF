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
		void starting();

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
	std::vector<std::string> tokes = tokens(get_issue(),"=");

	if(tokes.size() > 8)
		throw Exception("Path nesting too deep.");

	if(tokes.size() > 0)
	{
		const auto it = std::remove(tokes.at(0).begin(),tokes.at(0).end(),' ');
		tokes.at(0).erase(it,tokes.at(0).end());
		key = tokes.at(0);
	}
	else throw Exception("Invalid syntax to assign a configuration variable.");

	if(tokes.size() > 1)
	{
		const auto it = std::remove(tokes.at(1).begin(),tokes.at(1).end(),' ');
		tokes.at(1).erase(it,tokes.at(1).end());
		val = tokes.at(1);
	}
}
