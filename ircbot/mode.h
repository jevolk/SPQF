/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Mode : public std::string
{
	bool has(const char &m) const    { return find(m) != std::string::npos; }
	bool empty() const               { return empty();                      }

	void add(const char &m)
	{
		if(!has(m))
			push_back(m);
	}

	void rm(const char &m)
	{
		if(has(m))
			erase(find(m));
	}

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

	Mode(void): std::string() {}
	Mode(const std::string &mode): std::string(mode) {}
	Mode(const char *const &mode): std::string(mode) {}
};
