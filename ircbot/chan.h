/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Chan : public Locutor,
             public Acct
{
	Service *chanserv;
	Log _log;

	// State
	bool joined;                                            // Indication the server has sent us
	Mode _mode;
	time_t creation;
	std::string pass;                                       // passkey for channel
	std::map<std::string, std::string> info;
	std::tuple<std::string, Mask, time_t> _topic;
	std::unordered_map<std::string, std::tuple<User *, Mode>> users;
	std::set<Ban> bans;
	std::set<Quiet> quiets;
	std::set<Except> excepts;
	std::set<Invite> invites;
	std::set<AKick> akicks;
	std::set<Flag> flags;

  public:
	enum Type { SECRET, PRIVATE, PUBLIC };
	static char flag2mode(const char &flag);                // input = '@' then output = 'o' (or null)
	static char nick_flag(const std::string &name);         // input = "@nickname" then output = '@' (or null)
	static Type chan_type(const char &c);

	// Observers
	auto &get_cs() const                                    { return *chanserv;                     }
	auto &get_log() const                                   { return _log;                          }
	auto &get_name() const                                  { return Locutor::get_target();         }
	auto &get_pass() const                                  { return pass;                          }
	auto &is_joined() const                                 { return joined;                        }
	auto &get_topic() const                                 { return _topic;                        }
	auto &get_mode() const                                  { return _mode;                         }
	auto &get_creation() const                              { return creation;                      }
	auto &get_info() const                                  { return info;                          }
	auto &get_bans() const                                  { return bans;                          }
	auto &get_quiets() const                                { return quiets;                        }
	auto &get_excepts() const                               { return excepts;                       }
	auto &get_invites() const                               { return invites;                       }
	auto &get_flags() const                                 { return flags;                         }
	auto &get_akicks() const                                { return akicks;                        }
	uint num_users() const                                  { return users.size();                  }
	bool has_nick(const std::string &nick) const            { return users.count(nick);             }
	bool has(const User &user) const                        { return has_nick(user.get_nick());     }
	auto &get_user(const std::string &nick) const;
	auto &get_mode(const User &user) const;
	bool is_op() const;

	bool operator==(const Chan &o) const                    { return get_name() == o.get_name();    }
	bool operator!=(const Chan &o) const                    { return get_name() != o.get_name();    }
	bool operator==(const std::string &name) const          { return get_name() == name;            }
	bool operator!=(const std::string &name) const          { return get_name() != name;            }

	// Closures
	void for_each(const std::function<void (const User &, const Mode &)> &c) const;
	void for_each(const std::function<void (const User &)> &c) const;
	void for_each(const std::function<void (User &, Mode &)> &c);
	void for_each(const std::function<void (User &)> &c);

	// Closure recipes
	size_t count_logged_in() const;

  private:
	// [RECV] Handler may call these to update state
	friend class Bot;
	friend class ChanServ;

	auto &get_cs()                                          { return *chanserv;                     }
	auto &get_log()                                         { return _log;                          }
	auto &get_topic()                                       { return _topic;                        }

	void log(const User &user, const Msg &msg)              { _log(user,msg);                       }
	void set_joined(const bool &joined)                     { this->joined = joined;                }
	void set_creation(const time_t &creation)               { this->creation = creation;            }
	void delta_mode(const std::string &d)                   { _mode.delta(d);                       }
	bool delta_mode(const std::string &d, const Mask &m);
	void delta_flag(const std::string &d, const Mask &m);

	void set_info(const decltype(info) &info)               { this->info = info;                    }
	void set_flags(const decltype(flags) &flags)            { this->flags = flags;                  }
	void set_akicks(const decltype(akicks) &akicks)         { this->akicks = akicks;                }

	auto &get_user(const std::string &nick);
	auto &get_mode(const std::string &nick)                 { return std::get<1>(users.at(nick));   }
	auto &get_mode(const User &user)                        { return get_mode(user.get_nick());     }

	bool rename(const User &user, const std::string &old_nick);
	bool add(User &user, const Mode &mode = {});
	bool del(User &user);

  public:
	// [SEND] State update interface
	void who(const std::string &fl = User::WHO_FORMAT);     // Update state of users in channel (goes into Users->User)
	void accesslist();                                      // ChanServ access list update
	void flagslist();                                       // ChanServ flags list update
	void akicklist();                                       // ChanServ akick list update
	void invitelist();                                      // INVEX update
	void exceptlist();                                      // EXCEPTS update
	void quietlist()                                        { mode("+q");                           }
	void banlist()                                          { mode("+b");                           }
	void csinfo();                                          // ChanServ info update
	void names();                                           // Update user list of channel (goes into this->users)

	// [SEND] ChanServ interface to channel
	void csclear(const Mode &mode = "bq");                  // clear a list with a Mode vector
	void akick_del(const Mask &mask);
	void akick_del(const User &user);
	void akick(const Mask &mask, const std::string &ts = "", const std::string &reason = "");
	void akick(const User &user, const std::string &ts = "", const std::string &reason = "");
	void csunquiet(const Mask &mask);
	void csunquiet(const User &user);
	void csquiet(const Mask &user);
	void csquiet(const User &user);
	void csdeop(const User &user);
	void csop(const User &user);
	void recover();                                         // recovery procedure
	void unban();                                           // unban self
	void csdeop();                                          // target is self
	void op();                                              // target is self

	// [SEND] Direct interface to channel
	void knock(const std::string &msg = "");
	void invite(const std::string &nick);
	void topic(const std::string &topic);
	void kick(const User &user, const std::string &reason = "");
	void remove(const User &user, const std::string &reason = "");
	void devoice(const User &user);
	void voice(const User &user);
	uint unquiet(const User &user);
	bool quiet(const User &user, const Quiet::Type &type);
	bool quiet(const User &user);
	uint unban(const User &user);
	bool ban(const User &user, const Ban::Type &type);
	bool ban(const User &user);
	void part();                                            // Leave channel
	void join();                                            // Enter channel

	friend Chan &operator<<(Chan &c, const User &user);     // append "nickname: " to locutor stream
	friend User &operator<<(User &u, const Chan &chan);     // for CNOTICE / CPRIVMSG

	Chan(Adb &adb, Sess &sess, Service &chanserv, const std::string &name, const std::string &pass = "");
	virtual ~Chan() = default;

	friend std::ostream &operator<<(std::ostream &s, const Chan &chan);
};


inline
Chan::Chan(Adb &adb,
           Sess &sess,
           Service &chanserv,
           const std::string &name,
           const std::string &pass):
Locutor(sess,name),
Acct(adb,&Locutor::get_target()),
chanserv(&chanserv),
_log(sess,name),
joined(false),
creation(0),
pass(pass)
{


}


inline
User &operator<<(User &user,
                 const Chan &chan)
{
	const Sess &sess = chan.get_sess();

	if(!chan.is_op())
		return user;

	if(user.get_meth() == user.NOTICE && !sess.isupport("CNOTICE"))
		return user;

	if(user.get_meth() == user.PRIVMSG && !sess.isupport("CPRIVMSG"))
		return user;

	user.Stream::clear();
	user << user.CMSG;
	user << chan.get_target() << "\n";
	return user;
}


inline
Chan &operator<<(Chan &chan,
                 const User &user)
{
	chan << user.get_nick() << ": ";
	return chan;
}


inline
void Chan::join()
{
	Sess &sess = get_sess();
	const auto &pass = get_pass();
	sess.quote << "JOIN " << get_name();

	if(!pass.empty())
		sess.quote << " " << pass;

	sess.quote << flush;
}


inline
void Chan::part()
{
	Sess &sess = get_sess();
	sess.quote << "PART " << get_name() << flush;
}


inline
uint Chan::unquiet(const User &u)
{
	Deltas deltas;

	if(quiets.count(u.mask(Mask::NICK)))
		deltas.emplace_back('-','q',u.mask(Mask::NICK));

	if(quiets.count(u.mask(Mask::HOST)))
		deltas.emplace_back('-','q',u.mask(Mask::HOST));

	if(u.is_logged_in() && quiets.count(u.mask(Mask::ACCT)))
		deltas.emplace_back('-','q',u.mask(Mask::ACCT));

	mode(deltas);
	return deltas.size();
}


inline
uint Chan::unban(const User &u)
{
	Deltas deltas;

	if(bans.count(u.mask(Mask::NICK)))
		deltas.emplace_back('-','b',u.mask(Mask::NICK));

	if(bans.count(u.mask(Mask::HOST)))
		deltas.emplace_back('-','b',u.mask(Mask::HOST));

	if(u.is_logged_in() && bans.count(u.mask(Mask::ACCT)))
		deltas.emplace_back('-','b',u.mask(Mask::ACCT));

	mode(deltas);
	return deltas.size();
}


inline
bool Chan::ban(const User &u)
{
	Deltas deltas;
	deltas.emplace_back('+','b',u.mask(Mask::HOST));

	if(u.is_logged_in())
		deltas.emplace_back('+','b',u.mask(Mask::ACCT));

	mode(deltas);
	return true;
}


inline
bool Chan::quiet(const User &u)
{
	Deltas deltas;
	deltas.emplace_back('+','q',u.mask(Mask::HOST));

	if(u.is_logged_in())
		deltas.emplace_back('+','q',u.mask(Mask::ACCT));

	mode(deltas);
	return true;
}


inline
bool Chan::ban(const User &user,
               const Ban::Type &type)
{
	const Delta delta('+','b',user.mask(type));
	mode(delta);
	return true;
}


inline
bool Chan::quiet(const User &user,
                 const Quiet::Type &type)
{
	const Delta delta('+','q',user.mask(type));
	mode(delta);
	return true;
}


inline
void Chan::voice(const User &user)
{
	Sess &sess = get_sess();
	const std::string &targ = user.get_nick();
	const Delta delta('+','v',targ);
	mode(delta);
}


inline
void Chan::devoice(const User &user)
{
	Sess &sess = get_sess();
	const std::string &targ = user.get_nick();
	const Delta delta('-','v',targ);
	mode(delta);
}


inline
void Chan::remove(const User &user,
                  const std::string &reason)
{
	Sess &sess = get_sess();
	sess.quote << "REMOVE"
	           << " "  << get_name()
	           << " "  << user.get_nick()
	           << " :" << reason
	           << flush;
}


inline
void Chan::kick(const User &user,
                const std::string &reason)
{
	Sess &sess = get_sess();
	sess.quote << "KICK " << get_name() << " " << user.get_nick() << " :" << reason << flush;
}


inline
void Chan::invite(const std::string &nick)
{
	Sess &sess = get_sess();
	sess.quote << "INVITE " << nick << " " << get_name() << flush;
}


inline
void Chan::topic(const std::string &text)
{
	Sess &sess = get_sess();
	sess.quote << "TOPIC " << get_name();

	if(!text.empty())
		sess.quote << " :" << text;

	sess.quote << flush;
}


inline
void Chan::knock(const std::string &msg)
{
	Sess &sess = get_sess();
	sess.quote << "KNOCK " << get_name() << " :" << msg << flush;
}


inline
void Chan::op()
{
	Service &cs = get_cs();
	const Sess &sess = get_sess();
	cs << "OP " << get_name() << " " << sess.get_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::csdeop()
{
	Service &cs = get_cs();
	const Sess &sess = get_sess();
	cs << "DEOP " << get_name() << " " << sess.get_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::unban()
{
	Service &cs = get_cs();
	cs << "UNBAN " << get_name() << flush;
	cs.terminator_errors();
}


inline
void Chan::recover()
{
	Service &cs = get_cs();
	cs << "RECOVER " << get_name() << flush;
	cs.terminator_errors();
}


inline
void Chan::csop(const User &user)
{
	Service &cs = get_cs();
	cs << "OP " << get_name() << " " << user.get_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::csdeop(const User &user)
{
	Service &cs = get_cs();
	cs << "DEOP " << get_name() << " " << user.get_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::csquiet(const User &user)
{
	csquiet(user.mask(Mask::HOST));

//	if(user.is_logged_in())
//		csquiet(user.get_acct());
}


inline
void Chan::csunquiet(const User &user)
{
	csunquiet(user.mask(Mask::HOST));

//	if(user.is_logged_in())
//		csunquiet(user.get_acct());
}


inline
void Chan::csquiet(const Mask &mask)
{
	Service &cs = get_cs();
	cs << "QUIET " << get_name() << " " << mask << flush;
	cs.terminator_errors();
}


inline
void Chan::csunquiet(const Mask &mask)
{
	Service &cs = get_cs();
	cs << "UNQUIET " << get_name() << " " << mask << flush;
	cs.terminator_errors();
}


inline
void Chan::akick(const User &user,
                 const std::string &ts,
                 const std::string &reason)
{
	akick(user.mask(Mask::HOST),ts,reason);

	if(user.is_logged_in())
		akick(user.mask(Mask::ACCT),ts,reason);
}


inline
void Chan::akick(const Mask &mask,
                 const std::string &ts,
                 const std::string &reason)
{
	Service &cs = get_cs();
	cs << "AKICK " << get_name() << " ADD " << mask;

	if(!ts.empty())
		cs << " !T " << ts;
	else
		cs << " !P";

	cs << " " << reason;
	cs << flush;
	cs.terminator_any();
}


inline
void Chan::akick_del(const Mask &mask)
{
	Service &cs = get_cs();
	cs << "AKICK " << get_name() << " DEL " << mask << flush;
	cs.terminator_any();
}


inline
void Chan::csclear(const Mode &mode)
{
	Service &cs = get_cs();
	cs << "clear " << get_name() << " BANS " << mode << flush;
	cs.terminator_any();
}


inline
void Chan::csinfo()
{
	Service &cs = get_cs();
	cs << "info " << get_name() << flush;
	cs.terminator_next("*** End of Info ***");
}


inline
void Chan::names()
{
	Sess &sess = get_sess();
	sess.quote << "NAMES " << get_name() << flush;
}


inline
void Chan::invitelist()
{
	const auto &sess = get_sess();
	const auto &isupport = sess.get_isupport();
	mode(std::string("+") + isupport.get("INVEX",'I'));
}


inline
void Chan::exceptlist()
{
	const auto &sess = get_sess();
	const auto &isupport = sess.get_isupport();
	mode(std::string("+") + isupport.get("EXCEPTS",'e'));
}


inline
void Chan::flagslist()
{
	Service &cs = get_cs();
	cs << "flags " << get_name() << flush;

	std::stringstream ss;
	ss << "End of " << get_name() << " FLAGS listing.";
	cs.terminator_next(ss.str());
}


inline
void Chan::accesslist()
{
	Service &cs = get_cs();
	cs << "access " << get_name() << " list" << flush;

	std::stringstream ss;
	ss << "End of " << get_name() << " FLAGS listing.";
	cs.terminator_next(ss.str());
}


inline
void Chan::akicklist()
{
	Service &cs = get_cs();
	cs << "akick " << get_name() << " list" << flush;

	// This is the best we can do right now
	std::stringstream ss;
	ss << "Total of ";
	cs.terminator_next(ss.str());
}


inline
void Chan::who(const std::string &flags)
{
	Sess &sess = get_sess();
	sess.quote << "WHO " << get_name() << " " << flags << flush;
}


inline
auto &Chan::get_user(const std::string &nick)
try
{
	return *std::get<0>(users.at(nick));
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
auto &Chan::get_user(const std::string &nick)
const try
{
	return *std::get<0>(users.at(nick));
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
auto &Chan::get_mode(const User &user)
const
{
	return std::get<1>(users.at(user.get_nick()));
}


inline
void Chan::delta_flag(const std::string &delta,
                      const Mask &mask)
{
	auto it = flags.find({mask});
	if(it == flags.end())
		it = flags.emplace(mask).first;

	Flag &f = const_cast<Flag &>(*it);
	f.delta(delta);
	f.update(time(NULL));
}


inline
bool Chan::delta_mode(const std::string &delta,
                      const Mask &mask)
{
	bool ret;
	switch(hash(delta.c_str()))                // Careful what is switched if Mask::INVALID
	{
		case hash("+b"):     ret = bans.emplace(mask).second;    break;
		case hash("-b"):     ret = bans.erase(mask);             break;
		case hash("+q"):     ret = quiets.emplace(mask).second;  break;
		case hash("-q"):     ret = quiets.erase(mask);           break;
		default:             ret = false;                        break;
	}

	// Target is a straight nickname, not a Mask
	if(mask.get_form() == Mask::INVALID) try
	{
		const Sess &sess = get_sess();
		if(mask == sess.get_nick() && delta == "+o")
		{
			// We have been op'ed, grab the privileged lists.
			if(sess.isupport("INVEX"))
				invitelist();

			if(sess.isupport("EXCEPTS"))
				exceptlist();

			if(sess.has_opt("services"))
			{
				flagslist();
				akicklist();
			}
		}

		Mode &mode = get_mode(mask);
		return mode.delta(delta);
	}
	catch(const Exception &e)
	{
		// Ignore user's absence from channel, though this shouldn't happen.
		std::cerr << "Chan: " << get_name()
		          << " delta_mode(" << mask << "): "
		          << e << std::endl;
	}

	return ret;
}


inline
bool Chan::rename(const User &user,
                  const std::string &old_nick)
try
{
	auto val = users.at(old_nick);
	users.erase(old_nick);

	const auto &new_nick = user.get_nick();
	const auto iit = users.emplace(new_nick,val);
	return iit.second;
}
catch(const std::out_of_range &e)
{
	return false;
}


inline
bool Chan::del(User &user)
{
	const bool ret = users.erase(user.get_nick());

	if(ret)
		user.chans--;

	return ret;
}


inline
bool Chan::add(User &user,
               const Mode &mode)
{
	const auto iit = users.emplace(std::piecewise_construct,
	                               std::forward_as_tuple(user.get_nick()),
	                               std::forward_as_tuple(std::make_tuple(&user,mode)));
	const bool &ret = iit.second;

	if(ret)
		user.chans++;

	return ret;
}


inline
size_t Chan::count_logged_in()
const
{
	size_t ret = 0;
	for_each([&ret]
	(const User &user)
	{
		ret += user.is_logged_in();
	});

	return ret;
}


inline
void Chan::for_each(const std::function<void (User &)> &c)
{
	for(auto &p : users)
		c(*std::get<0>(std::get<1>(p)));
}


inline
void Chan::for_each(const std::function<void (const User &)> &c)
const
{
	for(const auto &p : users)
		c(*std::get<0>(std::get<1>(p)));
}


inline
void Chan::for_each(const std::function<void (User &, Mode &)> &c)
{
	for(auto &pair : users)
	{
		auto &val = pair.second;
		auto &user = *std::get<0>(val);
		auto &mode = std::get<1>(val);
		c(user,mode);
	}
}


inline
void Chan::for_each(const std::function<void (const User &, const Mode &)> &c)
const
{
	for(auto &pair : users)
	{
		const auto &val = pair.second;
		const auto &user = *std::get<0>(val);
		const auto &mode = std::get<1>(val);
		c(user,mode);
	}
}


inline
bool Chan::is_op()
const
{
	const Sess &sess = get_sess();
	const auto &user = get_user(sess.get_nick());
	const auto &mode = get_mode(user);
	return mode.has('o');
}


inline
Chan::Type Chan::chan_type(const char &c)
{
	switch(c)
	{
		case '@':     return SECRET;
		case '*':     return PRIVATE;
		case '=':
		default:      return PUBLIC;
	};
}


inline
char Chan::nick_flag(const std::string &name)
{
	const char &c = name.at(0);
	switch(c)
	{
		case '@':
		case '+':
			return c;

		default:
			return 0x00;
	}
}


inline
char Chan::flag2mode(const char &flag)
{
	switch(flag)
	{
		case '@':    return 'o';
		case '+':    return 'v';
		default:     return 0x00;
	}
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Chan &c)
{
	s << "name:       \t" << c.get_name() << std::endl;
	s << "passkey:    \t" << c.get_pass() << std::endl;
	s << "logfile:    \t" << c.get_log().get_path() << std::endl;
	s << "mode:       \t" << c.get_mode() << std::endl;
	s << "creation:   \t" << c.get_creation() << std::endl;
	s << "joined:     \t" << std::boolalpha << c.is_joined() << std::endl;
	s << "my privs:   \t" << (c.is_op()? "I am an operator." : "I am not an operator.") << std::endl;
	s << "topic:      \t" << std::get<std::string>(c.get_topic()) << std::endl;
	s << "topic by:   \t" << std::get<Mask>(c.get_topic()) << std::endl;
	s << "topic time: \t" << std::get<time_t>(c.get_topic()) << std::endl;

	s << "info:       \t" << c.info.size() << std::endl;
	for(const auto &kv : c.info)
		s << "\t" << kv.first << ":\t => " << kv.second << std::endl;

	s << "bans:      \t" << c.bans.size() << std::endl;
	for(const auto &b : c.bans)
		s << "\t+b " << b << std::endl;

	s << "quiets:    \t" << c.quiets.size() << std::endl;
	for(const auto &q : c.quiets)
		s << "\t+q " << q << std::endl;

	s << "excepts:   \t" << c.excepts.size() << std::endl;
	for(const auto &e : c.excepts)
		s << "\t+e " << e << std::endl;

	s << "invites:   \t" << c.invites.size() << std::endl;
	for(const auto &i : c.invites)
		s << "\t+I " << i << std::endl;

	s << "flags:     \t" << c.flags.size() << std::endl;
	for(const auto &f : c.flags)
		s << "\t"<< f << std::endl;

	s << "akicks:    \t" << c.akicks.size() << std::endl;
	for(const auto &a : c.akicks)
		s << "\t"<< a << std::endl;

	s << "users:      \t" << c.num_users() << std::endl;
	for(const auto &userp : c.users)
	{
		const auto &val = userp.second;
		const Mode &mode = std::get<1>(val);

		if(!mode.empty())
			s << "+" << mode;

		s << "\t" << userp.first << std::endl;;
	}
	s << std::endl;
	return s;
}
