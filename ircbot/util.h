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
std::string tolower(const std::string &str)
{
	static const std::locale locale = boost::locale::generator().generate("en_US.UTF-8");
	return boost::locale::to_lower(str.c_str(),locale);
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
