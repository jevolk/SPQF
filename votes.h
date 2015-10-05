/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */



///////////////////////////////////////////////////////////////////////////
//
// Common vote attributes
//


// The issue is a nickname
class NickIssue : virtual Vote
{
  protected:
	User user;        // Copy of the user at start of vote

  private:
	void event_nick(User &user, const std::string &old) override;
	void starting() override;

  protected:
	template<class... Args> NickIssue(Args&&... args);
};


// The effect is a reversible mode Delta
class ModeEffect : virtual Vote
{
	void expired() override;

  protected:
	template<class... Args> ModeEffect(Args&&... args): Vote(std::forward<Args>(args)...) {}
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

	class Mode : public virtual Vote,
	             public virtual ModeEffect
	{
		void passed() override;
		void starting() override;

	  public:
		template<class... Args> Mode(Args&&... args):
		                             Vote("mode",std::forward<Args>(args)...),
		                             ModeEffect("mode",std::forward<Args>(args)...) {}
	};

	class Kick : public virtual Vote,
	             public virtual NickIssue
	{
		void passed() override;

	  public:
		template<class... Args> Kick(Args&&... args):
		                             Vote("kick",std::forward<Args>(args)...),
		                             NickIssue("kick",std::forward<Args>(args)...) {}
	};

	class Invite : public Vote
	{
		void passed() override;

	  public:
		template<class... Args> Invite(Args&&... args):
		                               Vote("invite",std::forward<Args>(args)...) {}
	};

	class Topic : public Vote
	{
		void passed() override;
		void starting() override;

	  public:
		template<class... Args> Topic(Args&&... args):
		                              Vote("topic",std::forward<Args>(args)...) {}
	};

	class Opine : public Vote
	{
		void passed() override;

	  public:
		template<class... Args> Opine(Args&&... args):
		                              Vote("opine",std::forward<Args>(args)...) {}
	};

	class Ban : public virtual Vote,
	            public virtual NickIssue,
	            public virtual ModeEffect
	{
		void passed() override;

	  public:
		template<class... Args> Ban(Args&&... args):
		                            Vote("ban",std::forward<Args>(args)...),
		                            NickIssue("ban",std::forward<Args>(args)...),
		                            ModeEffect("ban",std::forward<Args>(args)...) {}
	};

	class Quiet : public virtual Vote,
	              public virtual NickIssue,
	              public virtual ModeEffect
	{
		void passed() override;

	  public:
		template<class... Args> Quiet(Args&&... args):
		                              Vote("quiet",std::forward<Args>(args)...),
		                              NickIssue("quiet",std::forward<Args>(args)...),
		                              ModeEffect("quiet",std::forward<Args>(args)...) {}
	};

	class UnQuiet : public virtual Vote,
	                public virtual NickIssue
	{
		void passed() override;

	  public:
		template<class... Args> UnQuiet(Args&&... args):
		                                Vote("unquiet",std::forward<Args>(args)...),
		                                NickIssue("unquiet",std::forward<Args>(args)...) {}
	};

	class Voice : public virtual Vote,
	              public virtual NickIssue,
	              public virtual ModeEffect
	{
		void passed() override;

	  public:
		template<class... Args> Voice(Args&&... args):
		                              Vote("voice",std::forward<Args>(args)...),
		                              NickIssue("voice",std::forward<Args>(args)...),
		                              ModeEffect("voice",std::forward<Args>(args)...) {}
	};

	class DeVoice : public virtual Vote,
	                public virtual NickIssue
	{
		void passed() override;

	  public:
		template<class... Args> DeVoice(Args&&... args):
		                                Vote("devoice",std::forward<Args>(args)...),
		                                NickIssue("devoice",std::forward<Args>(args)...) {}
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
		template<class... Args> Import(Args&&... args):
		                               Vote("import",std::forward<Args>(args)...) {}
	};
}



template<class... Args>
vote::Config::Config(Args&&... args):
Vote("config",std::forward<Args>(args)...)
{
	const auto tokes = tokens(get_issue()," = ");

	if(tokes.size() > 8)
		throw Exception("Path nesting too deep.");

	if(tokes.size() > 0)
		key = chomp(tokes.at(0)," ");
	else
		throw Exception("Invalid syntax to assign a configuration variable.");

	if(tokes.size() > 1)
		val = chomp(tokes.at(1)," ");
}



template<class... Args>
NickIssue::NickIssue(Args&&... args):
Vote(std::forward<Args>(args)...),
user([&]
{
	const auto &nick = get_issue();
	Users &users = get_users();
	return users.has(nick)? users.get(nick) : User(&get_adb(),&get_sess(),nullptr,nick);
}())
{

}
