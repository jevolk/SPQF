/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Mode
{
	std::string mode;

	size_t pos(const char &m) const          { return mode.find(m);                }

  public:
	const std::string &get() const           { return mode;                        }
	operator const std::string &() const     { return get();                       }

	bool empty() const                       { return get().empty();               }
	bool has(const char &m) const            { return pos(m) != std::string::npos; }
	void add(const char &m)                  { if(!has(m)) mode.push_back(m);      }
	void rm(const char &m)                   { if(has(m)) mode.erase(pos(m));      }

	bool delta(const std::string &str) try
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

	Mode(const std::string &mode = ""):
	     mode(mode) {}

	friend std::ostream &operator<<(std::ostream &s, const Mode &m)
	{
		s << m.get();
		return s;
	}
};
