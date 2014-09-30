/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Chan : public Locutor
{
  public:
	// Channel type and utils
	enum Type { SECRET, PRIVATE, PUBLIC };
	static char flag2mode(const char &flag);                // input = '@' then output = 'o' (or null)
	static char nick_flag(const std::string &name);         // input = "@nickname" then output = '@' (or null)
	static Type chan_type(const char &c);

	// Substructures
	struct Topic : std::tuple<std::string, Mask, time_t>    { enum { TEXT, MASK, TIME };            };
	using Quiets = Masks<Quiet>;
	using Bans = Masks<Ban>;

  private:
	using Userv = std::tuple<User *, Mode>;
	using Users = std::unordered_map<std::string, Userv>;

	Topic topic;
	Mode _mode;
	time_t creation;
	Users users;
	Quiets quiets;
	Bans bans;
	bool joined;                                            // State the server has sent us

  public:
	// Observers
	const std::string &get_name() const                     { return Locutor::get_target();         }
	const Topic &get_topic() const                          { return topic;                         }
	const Mode &get_mode() const                            { return _mode;                         }
	const time_t &get_creation() const                      { return creation;                      }
	const bool &is_joined() const                           { return joined;                        }
	const Quiets &get_quiets() const                        { return quiets;                        }
	const Bans &get_bans() const                            { return bans;                          }

	const User &get_user(const std::string &nick) const;
	const Mode &get_mode(const User &user) const;
	bool has_nick(const std::string &nick) const            { return users.count(nick);             }
	bool has(const User &user) const                        { return has_nick(user.get_nick());     }
	size_t num_users() const                                { return users.size();                  }
	bool is_op() const;

	// Closures
	void for_each(const std::function<void (const User &, const Mode &)> &c) const;
	void for_each(const std::function<void (const User &)> &c) const;
	void for_each(const std::function<void (User &, Mode &)> &c);
	void for_each(const std::function<void (User &)> &c);

  private:
	User &get_user(const std::string &nick);
	Mode &get_mode(const std::string &nick);
	Mode &get_mode(const User &user);

    // [RECV] Bot:: handler's call these to update state
	friend class Bot;
	void delta_mode(const std::string &d)                   { _mode.delta(d);                       }
	bool delta_mode(const std::string &d, const Mask &m);
	bool rename(const User &user, const std::string &old_nick);
	bool add(User &user, const Mode &mode = {});
	bool del(User &user);

	// [SEND] raw interfaces to channel
	void mode(const std::string &mode);                     // Raw mode command

  public:
	// [SEND] State update interface
	void who(const std::string &fl = User::WHO_FORMAT);     // Updates state of users in channel (goes into Users->User)
	void quietlist();                                       // Updates quietlist state of channel
	void banlist();                                         // Updates banlist state of channel
	void names();                                           // Update user list of channel (goes into this->users)
	void mode();                                            // Update mode of channel (sets this->mode)

	// [SEND] Control interface to channel
	void kick(const User &user, const std::string &reason = "");
	bool quiet(const User &user, const Quiet::Type &type = Quiet::Type::HOST);
	bool ban(const User &user, const Ban::Type &type = Ban::Type::HOST);
	size_t unquiet(const User &user);
	size_t unban(const User &user);

	void part();                                            // Leave channel
	void join();                                            // Enter channel

	Chan(Sess &sess, const std::string &name);
	~Chan() = default;

	friend std::ostream &operator<<(std::ostream &s, const Chan &chan);
};


inline
Chan::Chan(Sess &sess,
           const std::string &name):
Locutor(sess,name),
joined(false)
{


}


inline
void Chan::join()
{
	sess.call(irc_cmd_join,get_name().c_str(),nullptr);
}


inline
void Chan::part()
{
	sess.call(irc_cmd_part,get_name().c_str());
}


inline
void Chan::mode()
{
	sess.call(irc_cmd_channel_mode,get_name().c_str(),nullptr);
}


inline
void Chan::names()
{
	sess.call(irc_cmd_names,get_name().c_str());
}


inline
void Chan::quietlist()
{
	mode("+q");
}


inline
void Chan::banlist()
{
	mode("+b");
}


inline
void Chan::who(const std::string &flags)
{
	sess.quote("who %s %s",get_name().c_str(),flags.c_str());
}


inline
size_t Chan::unquiet(const User &u)
{
	std::stringstream m, s;
	m << "-";

	if(quiets.has(u.mask(Mask::NICK)))
	{
		m << "q";
		s << u.mask(Mask::NICK) << " ";
	}

	if(quiets.has(u.mask(Mask::HOST)))
	{
		m << "q";
		s << u.mask(Mask::HOST) << " ";
	}

	if(u.is_logged_in() && quiets.has(u.mask(Mask::ACCT)))
	{
		m << "q";
		s << u.mask(Mask::ACCT) << " ";
	}

	const size_t ret = m.str().size() - 1;

	if(ret)
	{
		m << " " << s.str();
		mode(m.str());
	}

	return ret;
}


inline
size_t Chan::unban(const User &u)
{
	std::stringstream m, s;
	m << "-";

	if(bans.has(u.mask(Mask::NICK)))
	{
		m << "b";
		s << u.mask(Mask::NICK) << " ";
	}

	if(bans.has(u.mask(Mask::HOST)))
	{
		m << "b";
		s << u.mask(Mask::HOST) << " ";
	}

	if(u.is_logged_in() && bans.has(u.mask(Mask::ACCT)))
	{
		m << "b";
		s << u.mask(Mask::ACCT) << " ";
	}

	const size_t ret = m.str().size() - 1;

	if(ret)
	{
		m << " " << s.str();
		mode(m.str());
	}

	return ret;
}


inline
bool Chan::ban(const User &user,
               const Ban::Type &type)
{
	std::stringstream s;
	s << "+b " << user.mask(type);
	mode(s.str());
	return true;
}


inline
bool Chan::quiet(const User &user,
                 const Quiet::Type &type)
{
	std::stringstream s;
	s << "+q " << user.mask(type);
	mode(s.str());
	return true;
}


inline
void Chan::kick(const User &user,
                const std::string &reason)
{
	const std::string &targ = user.get_nick();
	sess.call(irc_cmd_kick,targ.c_str(),get_name().c_str(),reason.c_str());
}


inline
void Chan::mode(const std::string &mode)
{
	sess.call(irc_cmd_channel_mode,get_name().c_str(),mode.c_str());
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
bool Chan::is_op()
const
{
	const std::string &nick = sess.get_nick();
	const User &user = get_user(nick);
	const Mode &mode = get_mode(user);
	return mode.has('o');
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
	s << "mode:       \t" << c.get_mode() << std::endl;
	s << "creation:   \t" << c.get_creation() << std::endl;
	s << "joined:     \t" << std::boolalpha << c.is_joined() << std::endl;
	s << "my privs:   \t" << (c.is_op()? "I am an operator." : "I am not an operator.") << std::endl;
	s << "topic:      \t" << std::get<Chan::Topic::TEXT>(c.get_topic()) << std::endl;
	s << "topic by:   \t" << std::get<Chan::Topic::MASK>(c.get_topic()) << std::endl;
	s << "topic time: \t" << std::get<Chan::Topic::TIME>(c.get_topic()) << std::endl;

	s << "users: " << c.num_users() << " -------" << std::endl;
	for(const auto &userp : c.users)
	{
		s << userp.first;

		const auto &val = userp.second;
		const Mode &mode = std::get<1>(val);
		if(!mode.empty())
			s << "(+" << mode << ")";

		s << " ";
	}
	s << std::endl;

	s << "Bans: " << c.bans.num() << " quiets: " << c.quiets.num() << " -------" << std::endl;
	for(const auto &b : c.bans)
		s << "+b " << b << std::endl;

	for(const auto &q : c.quiets)
		s << "+q " << q << std::endl;

	s << std::endl;
	return s;
}
