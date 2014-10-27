/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


enum Type
{
	SECRET,
	PRIVATE,
	PUBLIC
};

using Info = std::map<std::string, std::string>;
using Topic = std::tuple<std::string, Mask, time_t>;
template<class Value> using List = std::set<Value>;

Type type(const char &c);
char name_hat(const Server &serv, const std::string &nick); // "@nickname" then output = '@' (or null)


struct Lists
{
	List<Ban> bans;
	List<Quiet> quiets;
	List<Except> excepts;
	List<Invite> invites;
	List<AKick> akicks;
	List<Flag> flags;

	const Flag &get_flag(const Mask &m) const;
	const Flag &get_flag(const User &u) const;
	bool has_flag(const Mask &m) const;
	bool has_flag(const User &u) const;
	bool has_flag(const Mask &m, const char &flag) const;
	bool has_flag(const User &u, const char &flag) const;

	bool set_mode(const Delta &delta);
	void delta_flag(const Mask &m, const std::string &delta);

	friend std::ostream &operator<<(std::ostream &s, const Lists &lists);
};


class Users
{
	std::unordered_map<std::string, std::tuple<User *, Mode>> users;

  public:
	void for_each(const std::function<void (const User &, const Mode &)> &c) const;
	void for_each(const std::function<void (const User &)> &c) const;

	bool has(const std::string &nick) const                 { return users.count(nick);             }
	bool has(const User &user) const                        { return has(user.get_nick());          }
	auto &get(const std::string &nick) const;
	auto &mode(const std::string &nick) const               { return std::get<1>(users.at(nick));   }
	auto &mode(const User &user) const;
	auto num() const                                        { return users.size();                  }

	void for_each(const std::function<void (User &, Mode &)> &c);
	void for_each(const std::function<void (User &)> &c);
	size_t count_logged_in() const;

	auto &get(const std::string &nick);
	auto &mode(const std::string &nick)                     { return std::get<1>(users.at(nick));   }
	auto &mode(const User &user)                            { return mode(user.get_nick());         }
	bool rename(const User &user, const std::string &old);
	bool add(User &user, const Mode &mode = {});
	bool del(User &user);

	friend std::ostream &operator<<(std::ostream &s, const Users &users);
};


class Chan : public Locutor,
             public Acct
{
	Service *chanserv;
	Log _log;

	// State
	bool joined;                                            // Indication the server has sent us
	Mode _mode;                                             // Channel's mode state
	time_t creation;                                        // Timestamp for channel from server
	std::string pass;                                       // passkey for channel
	Topic _topic;                                           // Topic state
	Info info;                                              // ChanServ info response
	OpDo<Chan> opq;                                         // Queue of things to do when op'ed

  public:
	Users users;                                            // Users container direct interface
	Lists lists;                                            // Bans/Quiets/etc direct interface

	auto &get_name() const                                  { return Locutor::get_target();         }
	auto &get_cs() const                                    { return *chanserv;                     }
	auto &get_log() const                                   { return _log;                          }
	auto &is_joined() const                                 { return joined;                        }
	auto &get_mode() const                                  { return _mode;                         }
	auto &get_creation() const                              { return creation;                      }
	auto &get_pass() const                                  { return pass;                          }
	auto &get_topic() const                                 { return _topic;                        }
	auto &get_info() const                                  { return info;                          }
	auto &get_opq() const                                   { return opq;                           }

	bool has_mode(const char &mode) const                   { return get_mode().has(mode);          }
	bool is_op() const;

	bool operator==(const Chan &o) const                    { return get_name() == o.get_name();    }
	bool operator!=(const Chan &o) const                    { return get_name() != o.get_name();    }
	bool operator==(const std::string &name) const          { return get_name() == name;            }
	bool operator!=(const std::string &name) const          { return get_name() != name;            }

  protected:
	auto &get_cs()                                          { return *chanserv;                     }
	auto &get_log()                                         { return _log;                          }

	void run_opdo();
	void fetch_oplists();
	void event_opped();                                     // We have been opped up

  public:
	void log(const User &user, const Msg &msg)              { _log(user,msg);                       }
	void set_joined(const bool &joined)                     { this->joined = joined;                }
	void set_creation(const time_t &creation)               { this->creation = creation;            }
	void set_info(const decltype(info) &info)               { this->info = info;                    }
	auto &set_topic()                                       { return _topic;                        }
	bool set_mode(const Delta &d);
	template<class F> bool opdo(F&& f);                     // sudo <something as op> (happens async)

	// [SEND] State update interface
	void who(const std::string &fl = User::WHO_FORMAT);     // Update state of users in channel (goes into Users->User)
	void accesslist();                                      // ChanServ access list update
	void flagslist();                                       // ChanServ flags list update
	void akicklist();                                       // ChanServ akick list update
	void invitelist();                                      // INVEX update
	void exceptlist();                                      // EXCEPTS update
	void quietlist();                                       // 728 q list
	void banlist();
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
	void deop();                                            // target is self
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
	void quiet(const User &user, const Quiet::Type &type);
	void quiet(const User &user);
	uint unban(const User &user);
	void ban(const User &user, const Ban::Type &type);
	void ban(const User &user);
	void deop(const User &user);
	void op(const User &user);
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
	Quote out(get_sess(),"JOIN");
	out << get_name();

	if(!get_pass().empty())
		out << " " << get_pass();
}


inline
void Chan::part()
{
	Quote(get_sess(),"PART") << get_name();
}


inline
void Chan::op(const User &u)
{
	opdo(Delta("+o",u.get_nick()));
}


inline
void Chan::deop(const User &u)
{
	opdo(Delta("-o",u.get_nick()));
}


inline
uint Chan::unquiet(const User &u)
{
	Deltas deltas;

	if(lists.quiets.count(u.mask(Mask::NICK)))
		deltas.emplace_back("-q",u.mask(Mask::NICK));

	if(lists.quiets.count(u.mask(Mask::HOST)))
		deltas.emplace_back("-q",u.mask(Mask::HOST));

	if(u.is_logged_in() && lists.quiets.count(u.mask(Mask::ACCT)))
		deltas.emplace_back("-q",u.mask(Mask::ACCT));

	opdo(deltas);
	return deltas.size();
}


inline
uint Chan::unban(const User &u)
{
	Deltas deltas;

	if(lists.bans.count(u.mask(Mask::NICK)))
		deltas.emplace_back("-b",u.mask(Mask::NICK));

	if(lists.bans.count(u.mask(Mask::HOST)))
		deltas.emplace_back("-b",u.mask(Mask::HOST));

	if(u.is_logged_in() && lists.bans.count(u.mask(Mask::ACCT)))
		deltas.emplace_back("-b",u.mask(Mask::ACCT));

	opdo(deltas);
	return deltas.size();
}


inline
void Chan::ban(const User &u)
{
	Deltas deltas;
	deltas.emplace_back("+b",u.mask(Mask::HOST));

	if(u.is_logged_in())
		deltas.emplace_back("+b",u.mask(Mask::ACCT));

	opdo(deltas);
}


inline
void Chan::quiet(const User &u)
{
	Deltas deltas;
	deltas.emplace_back("+q",u.mask(Mask::HOST));

	if(u.is_logged_in())
		deltas.emplace_back("+q",u.mask(Mask::ACCT));

	opdo(deltas);
}


inline
void Chan::ban(const User &user,
               const Ban::Type &type)
{
	Deltas deltas;
	deltas.emplace_back("+b",user.mask(type));
	opdo(deltas);
}


inline
void Chan::quiet(const User &user,
                 const Quiet::Type &type)
{
	Deltas deltas;
	deltas.emplace_back("+q",user.mask(type));
	opdo(deltas);
}


inline
void Chan::voice(const User &user)
{
	Deltas deltas;
	deltas.emplace_back("+v",user.get_nick());
	opdo(deltas);
}


inline
void Chan::devoice(const User &user)
{
	Deltas deltas;
	deltas.emplace_back("-v",user.get_nick());
	opdo(deltas);
}


inline
void Chan::remove(const User &user,
                  const std::string &reason)
{
	const auto &nick = user.get_nick();
	opdo([nick,reason](Chan &chan)
	{
		Quote(chan.get_sess(),"REMOVE") << chan.get_name() << " "  << nick << " :" << reason;
	});
}


inline
void Chan::kick(const User &user,
                const std::string &reason)
{
	const auto &nick = user.get_nick();
	opdo([nick,reason](Chan &chan)
	{
		Quote(chan.get_sess(),"KICK") << chan.get_name() << " "  << nick << " :" << reason;
	});
}


inline
void Chan::invite(const std::string &nick)
{

	const auto func = [nick](Chan &chan)
	{
		Quote(chan.get_sess(),"INVITE") << chan.get_name() << " " << nick;
	};

	if(has_mode('g'))
		func(*this);
	else
		opdo(func);
}


inline
void Chan::topic(const std::string &text)
{
	opdo([text](Chan &chan)
	{
		Quote out(chan.get_sess(),"TOPIC");
		out << chan.get_name();

		if(!text.empty())
			out << " :" << text;
	});
}


inline
void Chan::knock(const std::string &msg)
{
	Quote(get_sess(),"KNOCK") << get_name() << " :" << msg;
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
void Chan::deop()
{
	mode(Delta("-o",get_my_nick()));
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
	Quote out(get_sess(),"NAMES");
	out << get_name() << flush;
}


inline
void Chan::banlist()
{
	const auto &sess = get_sess();
	const auto &serv = sess.get_server();
	if(serv.chan_pmodes.find('b') == std::string::npos)
		return;

	mode("+b");
}


inline
void Chan::quietlist()
{
	const auto &sess = get_sess();
	const auto &serv = sess.get_server();
	if(serv.chan_pmodes.find('q') == std::string::npos)
		return;

	mode("+q");
}


inline
void Chan::exceptlist()
{
	const auto &sess = get_sess();
	const auto &isup = sess.get_isupport();
	mode(std::string("+") + isup.get("EXCEPTS",'e'));
}


inline
void Chan::invitelist()
{
	const auto &sess = get_sess();
	const auto &isup = sess.get_isupport();
	mode(std::string("+") + isup.get("INVEX",'I'));
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
	Quote out(get_sess(),"WHO");
	out << get_name() << " " << flags << flush;
}


inline
bool Chan::set_mode(const Delta &delta)
try
{
	const auto &mask = std::get<Delta::MASK>(delta);
	const auto &sign = std::get<Delta::SIGN>(delta);
	const auto &mode = std::get<Delta::MODE>(delta);

	// Mode is for channel (TODO: special arguments)
	if(mask.empty())
	{
		_mode += delta;
		return true;
	}

	// Target is a straight nickname, not a Mask
	if(mask == Mask::INVALID)
	{
		if(mask == get_my_nick() && sign && mode == 'o')
			event_opped();

		users.mode(mask) += delta;
		return true;
	}

	return lists.set_mode(delta);
}
catch(const Exception &e)
{
	// Ignore user's absence from channel, though this shouldn't happen.
	std::cerr << "Chan: " << get_name() << " "
	          << "set_mode: " << delta << " "
	          << e << std::endl;

	return false;
}


template<class F>
bool Chan::opdo(F&& f)
{
	if(!is_op() && opq.empty())
		op();

	opq(std::forward<F>(f));

	if(!is_op())
		return false;

	run_opdo();
	return true;
}


inline
void Chan::event_opped()
{
	if(opq.empty())
		fetch_oplists();

	run_opdo();
}


inline
void Chan::fetch_oplists()
{
	const Sess &sess = get_sess();

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


inline
void Chan::run_opdo()
{
	const scope clear([&]
	{
		opq.clear();
	});

	for(const auto &lambda : opq.get_lambdas()) try
	{
		lambda(*this);
	}
	catch(const Exception &e)
	{
		std::cerr << "Chan::run_opdo() lambda exception: " << e << std::endl;
	}

	auto &deltas = opq.get_deltas();

	const auto &sess = get_sess();
	const auto &acct = sess.get_acct();
	if(!acct.empty() && lists.has_flag(acct) && !lists.has_flag(acct,'O'))
		deltas.emplace_back("-o",sess.get_nick());

	mode(deltas);
}


inline
bool Chan::is_op()
const
{
	const auto &sess = get_sess();
	const auto &mode = users.mode(sess.get_nick());
	return mode.has('o');
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

	s << c.lists << std::endl;
	s << c.users << std::endl;
	return s;
}



inline
bool Users::del(User &user)
{
	return users.erase(user.get_nick());
}


inline
bool Users::add(User &user,
                const Mode &mode)
{
	const auto iit = users.emplace(std::piecewise_construct,
	                               std::forward_as_tuple(user.get_nick()),
	                               std::forward_as_tuple(std::make_tuple(&user,mode)));
	return iit.second;
}


inline
bool Users::rename(const User &user,
                   const std::string &old)
try
{
	auto val = users.at(old);
	users.erase(old);

	const auto &new_nick = user.get_nick();
	const auto iit = users.emplace(new_nick,val);
	return iit.second;
}
catch(const std::out_of_range &e)
{
	return false;
}


inline
auto &Users::get(const std::string &nick)
try
{
	return *std::get<0>(users.at(nick));
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
auto &Users::get(const std::string &nick)
const try
{
	return *std::get<0>(users.at(nick));
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
void Users::for_each(const std::function<void (User &)> &c)
{
	for(auto &p : users)
		c(*std::get<0>(std::get<1>(p)));
}


inline
void Users::for_each(const std::function<void (User &, Mode &)> &c)
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
auto &Users::mode(const User &user)
const
{
	return std::get<1>(users.at(user.get_nick()));
}


inline
size_t Users::count_logged_in()
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
void Users::for_each(const std::function<void (const User &)> &c)
const
{
	for(const auto &p : users)
		c(*std::get<0>(std::get<1>(p)));
}


inline
void Users::for_each(const std::function<void (const User &, const Mode &)> &c)
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
std::ostream &operator<<(std::ostream &s,
                         const Users &u)
{
	s << "users:      \t" << u.num() << std::endl;
	for(const auto &userp : u.users)
	{
		const auto &val = userp.second;
		const auto &mode = std::get<1>(val);

		if(!mode.empty())
			s << "+" << mode;

		s << "\t" << userp.first << std::endl;
	}

	return s;
}



inline
void Lists::delta_flag(const Mask &mask,
                       const std::string &delta)
{
	auto it = flags.find({mask});
	if(it == flags.end())
		it = flags.emplace(mask).first;

	auto &f = const_cast<Flag &>(*it);
	f.delta(delta);
	f.update(time(NULL));
}


inline
bool Lists::set_mode(const Delta &d)
{
	const auto &mask = std::get<Delta::MASK>(d);
	const auto &sign = std::get<Delta::SIGN>(d);
	const auto &mode = std::get<Delta::MODE>(d);

	switch(mode)
	{
		case 'b':     return sign? bans.emplace(mask).second:
		                           bans.erase(mask);

		case 'q':     return sign? quiets.emplace(mask).second:
		                           quiets.erase(mask);

		default:      return false;
	}
}


inline
bool Lists::has_flag(const User &u,
                     const char &flag)
const
{
	const auto &gacct = u.get_val("info.account");
	return !gacct.empty()? has_flag(gacct,flag) : has_flag(u.get_acct(),flag);
}


inline
bool Lists::has_flag(const Mask &m,
                     const char &flag)
const
{
	const auto it = flags.find({m});
	return it != flags.end()? it->get_flags().has(flag) : false;
}


inline
bool Lists::has_flag(const User &u)
const
{
	return has_flag(u.get_acct());
}


inline
bool Lists::has_flag(const Mask &m)
const
{
	return flags.count({m});
}


inline
const Flag &Lists::get_flag(const User &u)
const
{
	return get_flag(u.get_acct());
}


inline
const Flag &Lists::get_flag(const Mask &m)
const
{
	const auto it = flags.find({m});
	if(it == flags.end())
		throw Exception("No flags matching this user.");

	return *it;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Lists &l)
{
	s << "bans:      \t" << l.bans.size() << std::endl;
	for(const auto &b : l.bans)
		s << "\t+b " << b << std::endl;

	s << "quiets:    \t" << l.quiets.size() << std::endl;
	for(const auto &q : l.quiets)
		s << "\t+q " << q << std::endl;

	s << "excepts:   \t" << l.excepts.size() << std::endl;
	for(const auto &e : l.excepts)
		s << "\t+e " << e << std::endl;

	s << "invites:   \t" << l.invites.size() << std::endl;
	for(const auto &i : l.invites)
		s << "\t+I " << i << std::endl;

	s << "flags:     \t" << l.flags.size() << std::endl;
	for(const auto &f : l.flags)
		s << "\t"<< f << std::endl;

	s << "akicks:    \t" << l.akicks.size() << std::endl;
	for(const auto &a : l.akicks)
		s << "\t"<< a << std::endl;

	return s;
}



inline
Type type(const char &c)
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
char name_hat(const Server &serv,
              const std::string &nick)
try
{
	const char &c = nick.at(0);
	return serv.has_prefix(c)? c : '\0';
}
catch(const std::out_of_range &e)
{
	return '\0';
}
