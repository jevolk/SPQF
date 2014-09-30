/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class User
{
	Sess &sess;

	// Handlers access
	friend class Bot;
	std::string nick;                                   // who 'n'
	std::string host;                                   // who 'h'
	std::string acct;                                   // who 'a'
	bool secure;                                        // WHOISSECURE
	time_t idle;                                        // who 'l' or WHOISIDLE

	// Chan increments or decrements
	friend class Chan;
	size_t chans;

  public:
	// WHO recipe with expected format, fulfills our members
	static constexpr int WHO_RECIPE = 0;
	static constexpr const char *WHO_FORMAT = "%tnha,0";

	// Observers
	const Sess &get_sess() const                        { return sess;                               }
	const std::string &get_nick() const                 { return nick;                               }
	const std::string &get_host() const                 { return host;                               }
	const std::string &get_acct() const                 { return acct;                               }
	const bool &is_secure() const                       { return secure;                             }
	const time_t &get_idle() const                      { return idle;                               }
	const size_t &num_chans() const                     { return chans;                              }

	bool is_logged_in() const                           { return acct.size() && acct != "0";         }
	bool is_myself() const                              { return get_nick() == sess.get_nick();      }

	Mask mask(const Mask::Type &t) const;               // Generate a mask from *this members

	// [SEND] Controls
	void notice(const std::string &msg);                // Notice to user
	void msg(const std::string &msg);                   // Message to user
	void who(const std::string &flags = WHO_FORMAT);    // Requests who with flags we need by default
	void whois();                                       // Requests full/multipart whois

	User(Sess &sess, const std::string &nick);

	bool operator<(const User &o) const                 { return nick < o.nick;                      }
	bool operator==(const User &o) const                { return nick == o.nick;                     }

	friend std::ostream &operator<<(std::ostream &s, const User &u);
};


inline
User::User(Sess &sess,
           const std::string &nick):
sess(sess),
nick(nick),
secure(false),
idle(0),
chans(0)
{


}


inline
void User::whois()
{
	sess.call(irc_cmd_whois,get_nick().c_str());
}


inline
void User::who(const std::string &flags)
{
    sess.quote("who %s %s",get_nick().c_str(),flags.c_str());
}


inline
void User::msg(const std::string &text)
{
	sess.call(irc_cmd_msg,get_nick().c_str(),text.c_str());
}


inline
void User::notice(const std::string &text)
{
	sess.call(irc_cmd_notice,get_nick().c_str(),text.c_str());
}


inline
Mask User::mask(const Mask::Type &recipe) const
{
	std::stringstream s;
	switch(recipe)
	{
		case Mask::NICK:
			s << get_nick() << "!*@*";
			break;

		case Mask::HOST:
			s << "*!*@" << get_host();
			break;

		case Mask::ACCT:
			if(!is_logged_in())
				throw Exception("Can't mask ACCT: user not logged in");

			s << "$a:" << get_acct();
			break;
	}

	return s.str();
}


inline
std::ostream &operator<<(std::ostream &s,
                         const User &u)
{
	s << std::setw(18) << std::left << u.get_nick();
	s << "@: " << std::setw(64) << std::left << u.get_host();
	s << "ssl: " << std::boolalpha << std::setw(8) << std::left << u.is_secure();
	s << "idle: " << std::setw(7) << std::left <<  u.get_idle();
	s << "chans: " << std::setw(3) << std::left << u.num_chans();

	if(u.is_logged_in())
		s << "acct: " << std::setw(18) << std::left << u.get_acct();

	if(u.is_myself())
		s << "<MYSELF>";

	return s;
}
