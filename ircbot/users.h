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
	std::unordered_map<std::string, std::unique_ptr<User>> users;

  public:
	// Observers
	const Adb &get_adb() const                         { return adb;                                }
	const Sess &get_sess() const                       { return sess;                               }

	const User &get(const std::string &nick) const;
	bool has(const std::string &nick) const            { return users.count(nick);                  }
	size_t num() const                                 { return users.size();                       }

	// Closures
	void for_each(const std::function<void (const User &)> &c) const;
	void for_each(const std::function<void (User &)> &c);

	// Manipulators
	void rename(const std::string &old_nick, const std::string &new_nick);
	User &get(const std::string &nick);
	User &add(const std::string &nick);
	bool del(const User &user);

	Users(Adb &adb, Sess &sess);

	friend std::ostream &operator<<(std::ostream &s, const Users &u);
};


inline
Users::Users(Adb &adb,
             Sess &sess):
adb(adb),
sess(sess)
{


}


inline
bool Users::del(const User &user)
{
	const std::string &nick = user.get_nick();
	return users.erase(nick);
}


inline
User &Users::add(const std::string &nick)
{
	const auto &iit = users.emplace(nick,std::unique_ptr<User>(new User(adb,sess,nick)));
	User &user = *iit.first->second;
	return user;
}


inline
User &Users::get(const std::string &nick)
try
{
	return *users.at(nick);
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
void Users::rename(const std::string &old_nick,
                   const std::string &new_nick)
{
	auto user = std::move(users.at(old_nick));
	users.erase(old_nick);
	user->set_nick(new_nick);
	users.emplace(new_nick,std::move(user));
}


inline
const User &Users::get(const std::string &nick)
const try
{
	return *users.at(nick);
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
void Users::for_each(const std::function<void (User &)> &closure)
{
	for(auto &userp : users)
	{
		User &user = *userp.second;
		closure(user);
	}
}


inline
void Users::for_each(const std::function<void (const User &)> &closure)
const
{
	for(const auto &userp : users)
	{
		const User &user = *userp.second;
		closure(user);
	}
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Users &u)
{
	s << "Users(" << u.num() << ")" << std::endl;
	for(const auto &userp : u.users)
	{
		const User &user = *userp.second;
		s << user << std::endl;
	}

	return s;
}
