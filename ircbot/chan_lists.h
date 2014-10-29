/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


template<class Value> using List = std::set<Value>;

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
	switch(char(d))
	{
		case 'b':     return d == '+'? bans.emplace(Mask(d)).second:
		                               bans.erase(Mask(d));

		case 'q':     return d == '+'? quiets.emplace(Mask(d)).second:
		                               quiets.erase(Mask(d));

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
