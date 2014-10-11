/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


constexpr
size_t hash(const char *const &str,
            const size_t i = 0)
{
    return !str[i]? 5381ULL : (hash(str,i+1) * 33ULL) ^ str[i];
}


inline
size_t hash(const std::string &str,
            const size_t i = 0)
{
    return i >= str.size()? 5381ULL : (hash(str,i+1) * 33ULL) ^ str.at(i);
}


template<class T>
std::string string(const T &t)
{
	std::stringstream s;
	s << t;
	return s.str();
}


template<class R,
         class I>
R pointers(I&& begin,
           I&& end)
{
	R ret;
	std::transform(begin,end,std::inserter(ret,ret.end()),[]
	(typename std::iterator_traits<I>::reference t)
	{
		return &t;
	});

	return ret;
}


template<class R,
         class C>
R pointers(C&& c)
{
	R ret;
	std::transform(c.begin(),c.end(),std::inserter(ret,ret.end()),[]
	(typename C::iterator::value_type &t)
	{
		return &t;
	});

	return ret;
}


inline
std::string chomp(const std::string &str,
                  const std::string &c = " ")
{
	const size_t pos = str.find_last_not_of(c);
	return pos == std::string::npos? str : str.substr(0,pos+1);
}


inline
std::pair<std::string, std::string> split(const std::string &str,
                                          const std::string &delim = " ")
{
	const size_t pos = str.find(delim);
	return pos == std::string::npos?
	              std::make_pair(str,std::string()):
	              std::make_pair(str.substr(0,pos), str.substr(pos+delim.size()));
}


inline
std::vector<std::string> tokens(const std::string &str,
                                const char *const &sep = " ")
{
	using delim = boost::char_separator<char>;

	const delim d(sep);
	const boost::tokenizer<delim> tk(str,d);
	return {tk.begin(),tk.end()};
}


inline
std::string tolower(const std::string &str)
{
	static const std::locale locale("en_US.UTF-8");

	std::string ret;
	std::transform(str.begin(),str.end(),std::back_inserter(ret),[]
	(const char &c)
	{
		return std::tolower(c,locale);
	});

	return ret;
}


inline
std::string decolor(const std::string &str)
{
	std::string ret(str);
	const auto end = std::remove_if(ret.begin(),ret.end(),[]
	(const char &c)
	{
		switch(c)
		{
			case '\x02':
			case '\x03':
				return true;

			default:
				return false;
		}
	});

	ret.erase(end,ret.end());
	return ret;
}


inline
std::string randstr(const size_t &len)
{
	std::string buf;
	buf.resize(len);
	std::generate(buf.begin(),buf.end(),[]
	{
		static const char *const randy = "abcdefghijklmnopqrstuvwxyz";
		return randy[rand() % strlen(randy)];
	});

	return buf;
}


struct scope
{
	typedef std::function<void ()> Func;
	const Func func;

	scope(const Func &func): func(func) {}
	~scope() { func(); }
};


class Exception : public std::runtime_error
{
	int c;

  public:
	const int &code() const  { return c; }

	Exception(const int &c, const std::string &what = ""):
	          std::runtime_error(what), c(c) {}

	Exception(const std::string &what = ""):
	          std::runtime_error(what), c(0) {}

	friend std::ostream &operator<<(std::ostream &s, const Exception &e)
	{
		s << e.what();
		return s;
	}
};


template<int CODE_FOR_SUCCESS = 0,
         class Exception = Exception,
         class Function,
         class... Args>
auto irc_call(irc_session_t *const &sess,
              Function&& func,
              Args&&... args)
-> decltype(func(sess,args...))
{
	const auto ret = func(sess,std::forward<Args>(args)...);

	if(ret != CODE_FOR_SUCCESS)
	{
		const int errc = irc_errno(sess);
		const char *const str = irc_strerror(errc);
		std::stringstream s;
		s << "libircclient: (" << errc << "): " << str;
		throw Exception(errc,s.str());
	}

	return ret;
}


template<int CODE_FOR_SUCCESS = 0,
         class Function,
         class... Args>
bool irc_call(std::nothrow_t,
              irc_session_t *const &sess,
              Function&& func,
              Args&&... args)
{
	const auto ret = func(sess,std::forward<Args>(args)...);
	return ret == CODE_FOR_SUCCESS;
}
