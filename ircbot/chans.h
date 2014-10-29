/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Chans
{
	Adb &adb;
	Sess &sess;
	Service *chanserv;

	using Cmp = CaseInsensitiveLess<std::string>;
	std::map<std::string, Chan, Cmp> chans;

  public:
	// Observers
	auto &get_adb() const                              { return adb;                                }
	auto &get_sess() const                             { return sess;                               }
	auto &get_cs() const                               { return *chanserv;                          }

	const Chan &get(const std::string &name) const     { return chans.at(name);                     }
	bool has(const std::string &name) const            { return chans.count(name);                  }
	auto num() const                                   { return chans.size();                       }

	// Closures
	void for_each(const std::function<void (const Chan &)> &c) const;
	void for_each(const std::function<void (Chan &)> &c);
	bool any_of(const std::function<bool (const Chan &)> &c) const;
	bool any_of(const std::function<bool (Chan &)> &c);

	Chan *find_cnotice(const User &user);              // returns null if not op in any channel with user

	// Manipulators
	Chan &get(const std::string &name);                // throws Exception
	Chan &add(const std::string &name);                // Add channel (w/o join) or return existing
	Chan &join(const std::string &name);               // Add channel with join or return existing
	bool del(const std::string &name)                  { return chans.erase(name);                  }
	bool del(const Chan &chan)                         { return del(chan.get_name());               }

	void servicejoin();                                // Joins all channels with access
	void autojoin();                                   // Joins all channels in the autojoin list

	// We construct before ChanServ; Bot sets this
	void set_service(Service &chanserv)                { this->chanserv = &chanserv;                }

	Chans(Adb &adb, Sess &sess, Service *const &chanserv = nullptr):
	      adb(adb), sess(sess), chanserv(chanserv) {}

	friend std::ostream &operator<<(std::ostream &s, const Chans &c);
};


inline
void Chans::autojoin()
{
	for(const auto &chan : sess.get_opts().autojoin)
		join(chan);
}


inline
void Chans::servicejoin()
{
	for(const auto &p : sess.get_access())
	{
		const auto &chan = p.first;
		const auto &mode = p.second;

		if(mode.has('A') && (mode.has('o') || mode.has('O')))
			join(chan);
	}
}


inline
Chan &Chans::join(const std::string &name)
{
	Chan &chan = add(name);
	chan.join();
	return chan;
}


inline
Chan &Chans::get(const std::string &name)
try
{
	return chans.at(name);
}
catch(const std::out_of_range &e)
{
	throw Exception() << "Unrecognized channel name [" << name << "]";
}


inline
Chan &Chans::add(const std::string &name)
{
	auto iit = chans.emplace(std::piecewise_construct,
	                         std::forward_as_tuple(tolower(name)),
	                         std::forward_as_tuple(&adb,&sess,chanserv,tolower(name)));
	return iit.first->second;
}


inline
Chan *Chans::find_cnotice(const User &user)
{
	Chan *ret = nullptr;
	any_of([&](Chan &chan)
	{
		if(!chan.is_op() || !chan.users.has(user))
			return false;

		ret = &chan;
		return true;
	});

	return ret;
}


inline
bool Chans::any_of(const std::function<bool (Chan &)> &closure)
{
	return std::any_of(chans.begin(),chans.end(),[&closure]
	(auto &chanp)
	{
		return closure(chanp.second);
	});
}


inline
bool Chans::any_of(const std::function<bool (const Chan &)> &closure)
const
{
	return std::any_of(chans.begin(),chans.end(),[&closure]
	(const auto &chanp)
	{
		return closure(chanp.second);
	});
}


inline
void Chans::for_each(const std::function<void (Chan &)> &closure)
{
	for(auto &chanp : chans)
		closure(chanp.second);
}


inline
void Chans::for_each(const std::function<void (const Chan &)> &closure)
const
{
	for(const auto &chanp : chans)
		closure(chanp.second);
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Chans &c)
{
	s << "Channels (" << c.num() << "): " << std::endl;
	for(const auto &chanp : c.chans)
		s << chanp.second << std::endl;

	return s;
}
