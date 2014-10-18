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
	return static_cast<std::stringstream &>(std::stringstream() << t).str();
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


template<class T>
bool isalpha(const T &val)
{
	return std::all_of(val.begin(),val.end(),[&]
	(auto&& c)
	{
		return std::isalpha(c,locale);
	});
}


template<class T>
bool isnumeric(const T &val)
{
	return std::all_of(val.begin(),val.end(),[&]
	(auto&& c)
	{
		return std::isdigit(c,locale);
	});
}


inline
std::string chomp(const std::string &str,
                  const std::string &c = " ")
{
	const auto pos = str.find_last_not_of(c);
	return pos == std::string::npos? str : str.substr(0,pos+1);
}


inline
std::pair<std::string, std::string> split(const std::string &str,
                                          const std::string &delim = " ")
{
	const auto pos = str.find(delim);
	return pos == std::string::npos?
	              std::make_pair(str,std::string()):
	              std::make_pair(str.substr(0,pos), str.substr(pos+delim.size()));
}


inline
std::string between(const std::string &str,
                    const std::string &a = "(",
                    const std::string &b = ")")
{
	return split(split(str,a).second,b).first;
}


template<template<class,class>
         class C = std::vector,
         class T = std::string,
         class A = std::allocator<T>>
C<T,A> tokens(const std::string &str,
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
	const int &code() const       { return c;      }
	operator std::string() const  { return what(); }

	Exception(const int &c, const std::string &what = ""): std::runtime_error(what), c(c) {}
	Exception(const std::stringstream &what): std::runtime_error(what.str()), c(0) {}
	Exception(const std::string &what = ""): std::runtime_error(what), c(0) {}

	template<class T> friend Exception operator<<(Exception &&e, const T &t)
	{
		return static_cast<std::stringstream &>(std::stringstream() << e << t);
	}

	friend std::ostream &operator<<(std::ostream &s, const Exception &e)
	{
		return (s << e.what());
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
		std::stringstream s;
		s << "libircclient: (" << errc << "): " << irc_strerror(errc);
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
	return func(sess,std::forward<Args>(args)...) == CODE_FOR_SUCCESS;
}
