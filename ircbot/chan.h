/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Chan : public Locutor,
             public Acct
{
  public:
	// Channel type and utils
	enum Type { SECRET, PRIVATE, PUBLIC };
	static char flag2mode(const char &flag);                // input = '@' then output = 'o' (or null)
	static char nick_flag(const std::string &name);         // input = "@nickname" then output = '@' (or null)
	static Type chan_type(const char &c);

	// Substructures
	struct Topic : std::tuple<std::string, Mask, time_t>    { enum { TEXT, MASK, TIME };            };
	using Info = std::map<std::string,std::string>;
	using Bans = Masks<Ban>;
	using Quiets = Masks<Quiet>;
	using Invites = Masks<Invite>;
	using Excepts = Masks<Except>;
	using Flags = Masks<Flag>;
	using AKicks = Masks<AKick>;

  private:
	using Userv = std::tuple<User *, Mode>;
	using Users = std::unordered_map<std::string, Userv>;

	// Facilities
	Service *cs;
	Log _log;

	// Primary state
	Topic _topic;
	Mode _mode;
	time_t creation;
	bool joined;                                            // Indication the server has sent us

	// Lists / extended data
	Info info;
	Bans bans;
	Quiets quiets;
	Excepts excepts;
	Invites invites;
	Flags flags;
	AKicks akicks;
	Users users;

  public:
	// Observers
	const Service &get_cs() const                           { return *cs;                           }
	const Log &get_log() const                              { return _log;                          }

	const std::string &get_name() const                     { return Locutor::get_target();         }
	const Topic &get_topic() const                          { return _topic;                        }
	const Mode &get_mode() const                            { return _mode;                         }
	const time_t &get_creation() const                      { return creation;                      }
	const bool &is_joined() const                           { return joined;                        }

	const Info &get_info() const                            { return info;                          }
	const Bans &get_bans() const                            { return bans;                          }
	const Quiets &get_quiets() const                        { return quiets;                        }
	const Excepts &get_excepts() const                      { return excepts;                       }
	const Invites &get_invites() const                      { return invites;                       }
	const Flags &get_flags() const                          { return flags;                         }
	const AKicks &get_akicks() const                        { return akicks;                        }

	const User &get_user(const std::string &nick) const;
	const Mode &get_mode(const User &user) const;
	bool has_nick(const std::string &nick) const            { return users.count(nick);             }
	bool has(const User &user) const                        { return has_nick(user.get_nick());     }
	uint num_users() const                                  { return users.size();                  }
	bool is_op() const;

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

	Service &get_cs()                                       { return *cs;                           }
	Log &get_log()                                          { return _log;                          }
	Topic &get_topic()                                      { return _topic;                        }

	void log(const User &user, const Msg &msg)              { _log(user,msg);                       }
	void set_joined(const bool &joined)                     { this->joined = joined;                }
	void set_creation(const time_t &creation)               { this->creation = creation;            }
	void delta_mode(const std::string &d)                   { _mode.delta(d);                       }
	bool delta_mode(const std::string &d, const Mask &m);

	void set_info(const Info &info)                         { this->info = info;                    }
	void set_flags(const Flags &flags)                      { this->flags = flags;                  }
	void set_akicks(const AKicks &akicks)                   { this->akicks = akicks;                }

	User &get_user(const std::string &nick);
	Mode &get_mode(const std::string &nick);
	Mode &get_mode(const User &user);
	bool rename(const User &user, const std::string &old_nick);
	bool add(User &user, const Mode &mode = {});
	bool del(User &user);

  public:
	// [SEND] State update interface
	void who(const std::string &fl = User::WHO_FORMAT);     // Update state of users in channel (goes into Users->User)
	void accesslist();                                      // ChanServ access list update
	void flagslist();                                       // ChanServ flags list update
	void akicklist();                                       // ChanServ akick list update
	void invitelist()                                       { mode("+I");                           }
	void exceptlist()                                       { mode("+e");                           }
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

	Chan(Adb &adb, Sess &sess, Service &cs, const std::string &name);
	virtual ~Chan() = default;

	friend std::ostream &operator<<(std::ostream &s, const Chan &chan);
};


inline
Chan::Chan(Adb &adb,
           Sess &sess,
           Service &cs,
           const std::string &name):
Locutor(sess,name),
Acct(adb,Locutor::get_target()),
cs(&cs),
_log(sess,name),
joined(false)
{


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
	sess.call(irc_cmd_join,get_name().c_str(),nullptr);
}


inline
void Chan::part()
{
	Sess &sess = get_sess();
	sess.call(irc_cmd_part,get_name().c_str());
}


inline
uint Chan::unquiet(const User &u)
{
	Deltas deltas;

	if(quiets.has(u.mask(Mask::NICK)))
		deltas.emplace_back('-','q',u.mask(Mask::NICK));

	if(quiets.has(u.mask(Mask::HOST)))
		deltas.emplace_back('-','q',u.mask(Mask::HOST));

	if(u.is_logged_in() && quiets.has(u.mask(Mask::ACCT)))
		deltas.emplace_back('-','q',u.mask(Mask::ACCT));

	mode(deltas);
	return deltas.size();
}


inline
uint Chan::unban(const User &u)
{
	Deltas deltas;

	if(bans.has(u.mask(Mask::NICK)))
		deltas.emplace_back('-','b',u.mask(Mask::NICK));

	if(bans.has(u.mask(Mask::HOST)))
		deltas.emplace_back('-','b',u.mask(Mask::HOST));

	if(u.is_logged_in() && bans.has(u.mask(Mask::ACCT)))
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
	const std::string &targ = user.get_nick();
	sess.quote("REMOVE %s %s :%s",get_name().c_str(),targ.c_str(),reason.c_str());
}


inline
void Chan::kick(const User &user,
                const std::string &reason)
{
	Sess &sess = get_sess();
	const std::string &targ = user.get_nick();
	sess.call(irc_cmd_kick,targ.c_str(),get_name().c_str(),reason.c_str());
}


inline
void Chan::invite(const std::string &nick)
{
	Sess &sess = get_sess();
	sess.call(irc_cmd_invite,nick.c_str(),get_name().c_str());
}


inline
void Chan::topic(const std::string &topic)
{
	Sess &sess = get_sess();
	sess.call(irc_cmd_topic,get_name().c_str(),topic.c_str());
}


inline
void Chan::op()
{
	Service &cs = get_cs();
	const Sess &sess = get_sess();
	cs << "OP " << get_name() << " " << sess.get_nick() << flush;
	cs.null_terminator();
}


inline
void Chan::csdeop()
{
	Service &cs = get_cs();
	const Sess &sess = get_sess();
	cs << "DEOP " << get_name() << " " << sess.get_nick() << flush;
	cs.null_terminator();
}


inline
void Chan::unban()
{
	Service &cs = get_cs();
	cs << "UNBAN " << get_name() << flush;
	cs.null_terminator();
}


inline
void Chan::recover()
{
	Service &cs = get_cs();
	cs << "RECOVER " << get_name() << flush;
	cs.null_terminator();
}


inline
void Chan::csop(const User &user)
{
	Service &cs = get_cs();
	cs << "OP " << get_name() << " " << user.get_nick() << flush;
	cs.null_terminator();
}


inline
void Chan::csdeop(const User &user)
{
	Service &cs = get_cs();
	cs << "DEOP " << get_name() << " " << user.get_nick() << flush;
	cs.null_terminator();
}


inline
void Chan::csquiet(const User &user)
{
	csquiet(user.mask(Mask::HOST));

	if(user.is_logged_in())
		csquiet(user.mask(Mask::ACCT));
}


inline
void Chan::csunquiet(const User &user)
{
	csunquiet(user.mask(Mask::HOST));

	if(user.is_logged_in())
		csunquiet(user.mask(Mask::ACCT));
}


inline
void Chan::csquiet(const Mask &mask)
{
	Service &cs = get_cs();
	cs << "QUIET " << get_name() << " " << mask << flush;
	cs.null_terminator();
}


inline
void Chan::csunquiet(const Mask &mask)
{
	Service &cs = get_cs();
	cs << "UNQUIET " << get_name() << " " << mask << flush;
	cs.null_terminator();
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
	cs.null_terminator();
}


inline
void Chan::akick_del(const Mask &mask)
{
	Service &cs = get_cs();
	cs << "AKICK " << get_name() << " DEL " << mask << flush;
	cs.null_terminator();
}


inline
void Chan::csclear(const Mode &mode)
{
	Service &cs = get_cs();
	cs << "clear " << get_name() << " BANS " << mode << flush;
	cs.null_terminator();
}


inline
void Chan::csinfo()
{
	Service &out = get_cs();
	out << "info " << get_name() << flush;
	out.next_terminator("*** End of Info ***");
}


inline
void Chan::names()
{
	Sess &sess = get_sess();
	sess.call(irc_cmd_names,get_name().c_str());
}


inline
void Chan::flagslist()
{
	Service &out = get_cs();
	out << "flags " << get_name() << flush;

	std::stringstream ss;
	ss << "End of " << get_name() << " FLAGS listing.";
	out.next_terminator(ss.str());
}


inline
void Chan::accesslist()
{
	Service &out = get_cs();
	out << "access " << get_name() << " list" << flush;

	std::stringstream ss;
	ss << "End of " << get_name() << " FLAGS listing.";
	out.next_terminator(ss.str());
}


inline
void Chan::akicklist()
{
	//TODO:  
	return;

	Service &out = get_cs();
	out << "akick " << get_name() << " list" << flush;

	std::stringstream ss;
	ss << "End of " << get_name() << " FLAGS listing.";
	out.next_terminator(ss.str());
}


inline
void Chan::who(const std::string &flags)
{
	Sess &sess = get_sess();
	sess.quote("who %s %s",get_name().c_str(),flags.c_str());
}


inline
bool Chan::delta_mode(const std::string &delta,
                      const Mask &mask)
{
	bool ret;
	switch(hash(delta.c_str()))                // Careful what is switched if Mask::INVALID
	{
		case hash("+b"):     ret = bans.add(mask);      break;
		case hash("-b"):     ret = bans.del(mask);      break;
		case hash("+q"):     ret = quiets.add(mask);    break;
		case hash("-q"):     ret = quiets.del(mask);    break;
		default:             ret = false;               break;
	}

	// Target is a straight nickname, not a Mask
	if(mask.get_form() == Mask::INVALID) try
	{
		const Sess &sess = get_sess();
		if(mask == sess.get_nick() && delta == "+o")
		{
			// We have been op'ed, grab the privileged lists.
			invitelist();
			exceptlist();
			flagslist();
			akicklist();
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
	Userv val = users.at(old_nick);
	users.erase(old_nick);

	const std::string &new_nick = user.get_nick();
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
	const std::string &nick = user.get_nick();
	const bool ret = users.erase(nick);

	if(ret)
		user.chans--;

	return ret;
}


inline
bool Chan::add(User &user,
               const Mode &mode)
{
	const std::string &nick = user.get_nick();
	const auto iit = users.emplace(std::piecewise_construct,
	                               std::forward_as_tuple(nick),
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
	for(auto &pair : users)
	{
		const std::string &nick = pair.first;
		Userv &val = pair.second;
		User &user = *std::get<0>(val);
		c(user);
	}
}


inline
void Chan::for_each(const std::function<void (const User &)> &c)
const
{
	for(const auto &pair : users)
	{
		const std::string &nick = pair.first;
		const Userv &val = pair.second;
		const User &user = *std::get<0>(val);
		c(user);
	}
}


inline
void Chan::for_each(const std::function<void (User &, Mode &)> &c)
{
	for(auto &pair : users)
	{
		const std::string &nick = pair.first;
		Userv &val = pair.second;
		User &user = *std::get<0>(val);
		Mode &mode = std::get<1>(val);
		c(user,mode);
	}
}


inline
void Chan::for_each(const std::function<void (const User &, const Mode &)> &c)
const
{
	for(const auto &pair : users)
	{
		const std::string &nick = pair.first;
		const Userv &val = pair.second;
		const User &user = *std::get<0>(val);
		const Mode &mode = std::get<1>(val);
		c(user,mode);
	}
}


inline
bool Chan::is_op()
const
{
	const Sess &sess = get_sess();
	const User &user = get_user(sess.get_nick());
	const Mode &mode = get_mode(user);
	return mode.has('o');
}


inline
Mode &Chan::get_mode(const User &user)
{
	const std::string &nick = user.get_nick();
	return get_mode(nick);
}


inline
Mode &Chan::get_mode(const std::string &nick)
{
	Userv &val = users.at(nick);
	return std::get<1>(val);
}


inline
User &Chan::get_user(const std::string &nick)
try
{
	Userv &val = users.at(nick);
	return *std::get<0>(val);
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
const Mode &Chan::get_mode(const User &user)
const
{
	const std::string &nick = user.get_nick();
	const Userv &val = users.at(nick);
	return std::get<1>(val);
}


inline
const User &Chan::get_user(const std::string &nick)
const try
{
	const Userv &val = users.at(nick);
	return *std::get<0>(val);
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
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
	s << "logfile:    \t" << c.get_log().get_path() << std::endl;
	s << "mode:       \t" << c.get_mode() << std::endl;
	s << "creation:   \t" << c.get_creation() << std::endl;
	s << "joined:     \t" << std::boolalpha << c.is_joined() << std::endl;
	s << "my privs:   \t" << (c.is_op()? "I am an operator." : "I am not an operator.") << std::endl;
	s << "topic:      \t" << std::get<Chan::Topic::TEXT>(c.get_topic()) << std::endl;
	s << "topic by:   \t" << std::get<Chan::Topic::MASK>(c.get_topic()) << std::endl;
	s << "topic time: \t" << std::get<Chan::Topic::TIME>(c.get_topic()) << std::endl;

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
		s << "akick: \t"<< a << std::endl;

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
