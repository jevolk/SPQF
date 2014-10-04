/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class User : public Locutor,
             public Acct
{
	// nick is Locutor::target                         // who 'n'
	std::string host;                                  // who 'h'
	std::string acct;                                  // who 'a' (account name)
	bool secure;                                       // WHOISSECURE (ssl)
	time_t signon;                                     // WHOISIDLE
	time_t idle;                                       // who 'l' or WHOISIDLE
	bool away;

	// Chan increments or decrements
	friend class Chan;
	size_t chans;

  public:
	static constexpr int WHO_RECIPE                    = 0;
	static constexpr const char *const WHO_FORMAT      = "%tnha,0";       // ID must match WHO_RECIPE

	// Observers
	const std::string &get_nick() const                { return Locutor::get_target();               }
	const std::string &get_host() const                { return host;                                }
	const std::string &get_acct() const                { return acct;                                }
	const bool &is_secure() const                      { return secure;                              }
	const bool &is_away() const                        { return away;                                }
	const time_t &get_signon() const                   { return signon;                              }
	const time_t &get_idle() const                     { return idle;                                }
	const size_t &num_chans() const                    { return chans;                               }

	bool is_myself() const                             { return get_nick() == get_sess().get_nick(); }
	bool is_logged_in() const                          { return acct.size() && acct != "0";          }
	Mask mask(const Mask::Type &t) const;              // Generate a mask from *this members

	// Mutators used by Bot handlers
	void set_acct(const std::string &acct)             { this->acct = tolower(acct);                 }
	void set_host(const std::string &host)             { this->host = host;                          }
	void set_secure(const bool &secure)                { this->secure = secure;                      }
	void set_signon(const time_t &signon)              { this->signon = signon;                      }
	void set_idle(const time_t &idle)                  { this->idle = idle;                          }
	void set_away(const bool &away)                    { this->away = away;                          }

	// [SEND] Controls
	void who(const std::string &flags = WHO_FORMAT);   // Requests who with flags we need by default

	User(Adb &adb, Sess &sess, const std::string &nick);

	bool operator<(const User &o) const                { return get_nick() < o.get_nick();           }
	bool operator==(const User &o) const               { return get_nick() == o.get_nick();          }

	friend std::ostream &operator<<(std::ostream &s, const User &u);
};


inline
User::User(Adb &adb,
           Sess &sess,
           const std::string &nick):
Locutor(sess,nick),
Acct(adb,this->acct),
secure(false),
signon(0),
idle(0),
away(false),
chans(0)
{


}


inline
void User::who(const std::string &flags)
{
	Sess &sess = get_sess();
	sess.quote("who %s %s",get_nick().c_str(),flags.c_str());
}


inline
Mask User::mask(const Mask::Type &recipe)
const
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
		s << "$a: " << std::setw(18) << std::left << u.get_acct();

	if(u.is_myself())
		s << "<MYSELF>";

	return s;
}