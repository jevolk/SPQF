/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Users
{
	using Cmp = CaseInsensitiveEqual<std::string>;
	std::unordered_map<std::string, std::tuple<User *, Mode>, Cmp, Cmp> users;

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
