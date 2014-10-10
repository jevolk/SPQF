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

	std::map<std::string, Chan> chans;

  public:
	// Observers
	const Adb &get_adb() const                         { return adb;                                }
	const Sess &get_sess() const                       { return sess;                               }

	const Chan &get(const std::string &name) const     { return chans.at(tolower(name));            }
	bool has(const std::string &name) const            { return chans.count(tolower(name));         }
	size_t num() const                                 { return chans.size();                       }

	// Closures
	void for_each(const std::function<void (const Chan &)> &c) const;
	void for_each(const std::function<void (Chan &)> &c);

	// Manipulators
	Chan &get(const std::string &name);                // throws Exception
	Chan &add(const std::string &name);                // Add channel (w/o join) or return existing
	Chan &join(const std::string &name);               // Add channel with join or return existing
	bool del(const std::string &name)                  { return chans.erase(tolower(name));         }
	bool del(const Chan &chan)                         { return del(chan.get_name());               }
	void autojoin();                                   // Joins all channels in the autojoin list

	Chans(Adb &adb, Sess &sess);

	friend std::ostream &operator<<(std::ostream &s, const Chans &c);
};


inline
Chans::Chans(Adb &adb,
             Sess &sess):
adb(adb),
sess(sess)
{


}


inline
void Chans::autojoin()
{
	const Ident &id = sess.get_ident();
	for(const auto &chan : id.autojoin)
		join(chan);
}


inline
Chan &Chans::join(const std::string &name)
{
	Chan &chan = add(tolower(name));
	chan.join();
	return chan;
}


inline
Chan &Chans::get(const std::string &name)
try
{
	return chans.at(tolower(name));
}
catch(const std::out_of_range &e)
{
	std::stringstream s;
	s << "Unrecognized channel name [" << tolower(name) << "]";
	throw Exception(s.str());
}


inline
Chan &Chans::add(const std::string &name)
{
	auto iit = chans.emplace(std::piecewise_construct,
	                         std::forward_as_tuple(tolower(name)),
	                         std::forward_as_tuple(adb,sess,tolower(name)));

	Chan &chan = iit.first->second;
	return chan;
}


inline
void Chans::for_each(const std::function<void (Chan &)> &closure)
{
	for(auto &chanp : chans)
	{
		Chan &chan = chanp.second;
		closure(chan);
	}
}


inline
void Chans::for_each(const std::function<void (const Chan &)> &closure)
const
{
	for(const auto &chanp : chans)
	{
		const Chan &chan = chanp.second;
		closure(chan);
	}
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
