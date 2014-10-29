/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Mode : public std::string
{
	bool has(const char &m) const                 { return find(m) != std::string::npos;  }
	bool has(const Delta &d) const                { return has(std::get<Delta::MODE>(d)); }
	bool none(const Mode &m) const;
	bool all(const Mode &m) const;
	bool any(const Mode &m) const;

	bool valid_chan(const Server &s) const;
	bool valid_user(const Server &s) const;

	bool add(const char &m) &;
	void add(const Mode &m) &;
	bool add(const Delta &d) &;                   // adding negative delta removes the mode
	void add(const Deltas &d) &;                  // ^

	bool rm(const char &m) &;
	void rm(const Mode &m) &;
	bool rm(const Delta &d) &;                    // rm of positive or negative delta always removes the mode
	void rm(const Deltas &d) &;                   // ^

	bool delta(const std::string &str) &;

	template<class T> Mode &operator+=(T&& m) &;  // add()
	template<class T> Mode &operator-=(T&& m) &;  // rm()
	template<class T> Mode operator+(T&& m);      // add()
	template<class T> Mode operator-(T&& m);      // rm()

	Mode(void): std::string() {}
	Mode(const std::string &mode);
	Mode(const char &mode): std::string(1,mode) {}
	Mode(const char *const &mode): Mode(std::string(mode)) {}
};


inline
Mode::Mode(const std::string &mode):
std::string(!mode.empty() && (mode.at(0) == '+' || mode.at(0) == '-')? mode.substr(1) : mode)
{


}


template<class T>
Mode Mode::operator-(T&& m)
{
	Mode ret(*this);
	ret -= std::forward<T>(m);
	return ret;
}


template<class T>
Mode Mode::operator+(T&& m)
{
	Mode ret(*this);
	ret += std::forward<T>(m);
	return ret;
}


template<class T>
Mode &Mode::operator-=(T&& m)
&
{
	rm(std::forward<T>(m));
	return *this;
}


template<class T>
Mode &Mode::operator+=(T&& m) &
{
	add(std::forward<T>(m));
	return *this;
}


inline
bool Mode::delta(const std::string &str)
& try
{
	if(str.at(0) == '-')
		for(size_t i = 1; i < str.size(); i++)
			rm(str.at(i));
	else
		for(size_t i = 1; i < str.size(); i++)
			add(str.at(i));

	return true;
}
catch(const std::out_of_range &e)
{
	return false;
}


inline
void Mode::add(const Deltas &deltas)
&
{
	for(const Delta &d : deltas)
		add(d);
}


inline
bool Mode::add(const Delta &d)
&
{
	using std::get;

	if(get<Delta::SIGN>(d))
		return add(get<Delta::MODE>(d));
	else
		return rm(get<Delta::MODE>(d));
}


inline
void Mode::add(const Mode &m)
&
{
	for(const char &c : m)
		add(c);
}


inline
bool Mode::add(const char &m)
&
{
	if(!has(m))
	{
		push_back(m);
		return true;
	}
	else return false;
}


inline
void Mode::rm(const Deltas &deltas)
&
{
	for(const Delta &d : deltas)
		rm(d);
}


inline
bool Mode::rm(const Delta &d)
&
{
	return rm(std::get<Delta::MODE>(d));
}


inline
void Mode::rm(const Mode &m)
&
{
	for(const char &c : m)
		rm(c);
}


inline
bool Mode::rm(const char &m)
&
{
	if(has(m))
	{
		erase(find(m));
		return true;
	}
	else return false;
}


inline
bool Mode::any(const Mode &m)
const
{
	return std::any_of(m.begin(),m.end(),[&]
	(const char &c)
	{
		return has(c);
	});
}


inline
bool Mode::all(const Mode &m)
const
{
	return std::all_of(m.begin(),m.end(),[&]
	(const char &c)
	{
		return has(c);
	});
}


inline
bool Mode::none(const Mode &m)
const
{
	return std::none_of(m.begin(),m.end(),[&]
	(const char &c)
	{
		return has(c);
	});
}
