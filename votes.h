/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */



///////////////////////////////////////////////////////////////////////////
//
// Vote categories / abstract bases
//

struct NickIssue : public Vote
{
	void event_nick(User &user, const std::string &old) override;
	void starting() override;

	template<class... Args> NickIssue(Args&&... args): Vote(std::forward<Args>(args)...) {}
};



///////////////////////////////////////////////////////////////////////////
//
// Vote types
//

namespace vote
{
	class Config : public Vote
	{
		std::string key;
		std::string val;

		void passed() override;
		void starting() override;

	  public:
		template<class... Args> Config(Args&&... args);
	};

	class Mode : public Vote
	{
		void passed() override;
		void starting() override;

	  public:
		template<class... Args> Mode(Args&&... args): Vote("mode",std::forward<Args>(args)...) {}
	};

	class Kick : public NickIssue
	{
		void passed() override;

	  public:
		template<class... Args> Kick(Args&&... args): NickIssue("kick",std::forward<Args>(args)...) {}
	};

	class Invite : public NickIssue
	{
		void passed() override;

	  public:
		template<class... Args> Invite(Args&&... args): NickIssue("invite",std::forward<Args>(args)...) {}
	};

	class Topic : public Vote
	{
		void passed() override;
		void starting() override;

	  public:
		template<class... Args> Topic(Args&&... args): Vote("topic",std::forward<Args>(args)...) {}
	};

	class Opine : public Vote
	{
		void passed() override;

	  public:
		template<class... Args> Opine(Args&&... args): Vote("opine",std::forward<Args>(args)...) {}
	};

	class Ban : public NickIssue
	{
		void passed() override;

	  public:
		template<class... Args> Ban(Args&&... args): NickIssue("ban",std::forward<Args>(args)...) {}
	};

	class Quiet : public NickIssue
	{
		void passed() override;

	  public:
		template<class... Args> Quiet(Args&&... args): NickIssue("quiet",std::forward<Args>(args)...) {}
	};

	class UnQuiet : public NickIssue
	{
		void passed() override;

	  public:
		template<class... Args> UnQuiet(Args&&... args): NickIssue("unquiet",std::forward<Args>(args)...) {}
	};

	class Import : public Vote
	{
		std::stringstream received;

		std::string get_target_chan() const  { return split(get_issue()).first;  }
		std::string get_target_bot() const   { return split(get_issue()).second; }

		void passed() override;
		void starting() override;
		void event_notice(User &u, const std::string &text) override;

	  public:
		template<class... Args> Import(Args&&... args): Vote("import",std::forward<Args>(args)...) {}
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
