/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Chans
{
	Sess &sess;

	std::map<std::string, Chan> chans;

  public:
	using ConstClosure = std::function<void (const Chan &)>;
	using Closure = std::function<void (Chan &)>;

	const Chan &get(const std::string &name) const     { return chans.at(name);                     }
	bool has(const std::string &name) const            { return chans.count(name);                  }
	size_t num() const                                 { return chans.size();                       }

	void for_each(const ConstClosure &c) const;
	void for_each(const Closure &c);

	Chan &get(const std::string &name);                // throws Exception
	bool del(const std::string &name)                  { return chans.erase(name);                  }
	Chan &add(const std::string &name);                // Add channel (w/o join) or return existing
	Chan &join(const std::string &name);               // Add channel with join or return existing

	Chans(Sess &sess);

	friend std::ostream &operator<<(std::ostream &s, const Chans &c);
};


inline
Chans::Chans(Sess &sess):
sess(sess)
{


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
	std::stringstream s;
	s << "Unrecognized channel name [" << name << "]";
	throw Exception(s.str());
}


inline
Chan &Chans::add(const std::string &name)
{
	auto iit = chans.emplace(std::piecewise_construct,
	                         std::forward_as_tuple(name),
	                         std::forward_as_tuple(sess,name));

	Chan &chan = iit.first->second;
	return chan;
}


inline
void Chans::for_each(const Closure &closure)
{
	for(auto &chanp : chans)
	{
		Chan &chan = chanp.second;
		closure(chan);
	}
}


inline
void Chans::for_each(const ConstClosure &closure) const
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
