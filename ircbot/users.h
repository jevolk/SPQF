/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Users
{
	Adb &adb;
	Sess &sess;
	Service *nickserv;

	using Cmp = CaseInsensitiveEqual<std::string>;
	std::unordered_map<std::string, User, Cmp, Cmp> users;

  public:
	// Observers
	auto &get_adb() const                                { return adb;                        }
	auto &get_sess() const                               { return sess;                       }
	auto &get_ns() const                                 { return *nickserv;                  }

	const User &get(const std::string &nick) const;
	bool has(const std::string &nick) const              { return users.count(nick);          }
	auto num() const                                     { return users.size();               }

	// Closures
	void for_each(const std::function<void (const User &)> &c) const;
	void for_each(const std::function<void (User &)> &c);

	// Manipulators
	void rename(const std::string &old_nick, const std::string &new_nick);
	User &get(const std::string &nick);
	User &add(const std::string &nick);
	bool del(const User &user);

	// We construct before NickServ; Bot sets this
	void set_service(Service &nickserv)                  { this->nickserv = &nickserv;        }

	Users(Adb &adb, Sess &sess, Service *const &nickserv = nullptr):
	      adb(adb), sess(sess), nickserv(nickserv) {}

	friend std::ostream &operator<<(std::ostream &s, const Users &u);
};


inline
bool Users::del(const User &user)
{
	return users.erase(user.get_nick());
}


inline
User &Users::add(const std::string &nick)
{
	const auto &iit = users.emplace(std::piecewise_construct,
	                                std::forward_as_tuple(nick),
	                                std::forward_as_tuple(&adb,&sess,nickserv,nick));
	return iit.first->second;
}


inline
User &Users::get(const std::string &nick)
try
{
	return users.at(nick);
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
void Users::rename(const std::string &old_nick,
                   const std::string &new_nick)
{
	User &old_user = users.at(old_nick);
	User tmp_user = std::move(old_user);
	tmp_user.set_nick(new_nick);
	users.erase(old_nick);
	users.emplace(new_nick,std::move(tmp_user));
}


inline
const User &Users::get(const std::string &nick)
const try
{
	return users.at(nick);
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
void Users::for_each(const std::function<void (User &)> &closure)
{
	for(auto &userp : users)
		closure(userp.second);
}


inline
void Users::for_each(const std::function<void (const User &)> &closure)
const
{
	for(const auto &userp : users)
		closure(userp.second);
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Users &u)
{
	s << "Users(" << u.num() << ")" << std::endl;
	for(const auto &userp : u.users)
		s << userp.second << std::endl;

	return s;
}
