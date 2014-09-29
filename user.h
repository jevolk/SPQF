/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class User
{
	Sess &sess;
	std::string nick;
	size_t chans;

  public:
	// Observers
	const Sess &get_sess() const                       { return sess;                               }
	const std::string &get_nick() const                { return nick;                               }
	bool is_self() const                               { return sess.get_nick() == get_nick();      }

	const size_t &num_chans() const                    { return chans;                              }
	size_t dec_chans()                                 { return chans--;                            }
	size_t inc_chans()                                 { return chans++;                            }

	// [SEND] Controls
	void kick(const std::string &chan, const std::string &reason = "");
	void notice(const std::string &msg);               // Notice to user
	void msg(const std::string &msg);                  // Message to user
	void whois();                                      // Sends whois to server

	User(Sess &sess, const std::string &nick);

	bool operator<(const User &o) const                { return nick < o.nick;                      }

	friend std::ostream &operator<<(std::ostream &s, const User &u);
};


inline
User::User(Sess &sess,
           const std::string &nick):
sess(sess),
nick(nick),
chans(0)
{


}


inline
void User::whois()
{
	sess.call(irc_cmd_whois,get_nick().c_str());
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
void User::kick(const std::string &chan,
                const std::string &reason)
{
	sess.call(irc_cmd_kick,get_nick().c_str(),chan.c_str(),reason.c_str());
}


inline
std::ostream &operator<<(std::ostream &s,
                         const User &u)
{
	s << "nick: " << u.get_nick() << " chans: " << u.num_chans();
	return s;
}
