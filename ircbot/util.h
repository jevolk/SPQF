/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


extern std::locale locale;   // bot.cpp


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


template<class T = std::string,
         class... Args>
auto lex_cast(Args&&... args)
{
	return boost::lexical_cast<T>(std::forward<Args>(args)...);
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


template<class I>
bool isnumeric(const I &beg,
               const I &end)
{
	return std::all_of(beg,end,[&]
	(auto&& c)
	{
		return std::isdigit(c,locale);
	});
}


template<class I>
bool isalpha(const I &beg,
             const I &end)
{
	return std::all_of(beg,end,[&]
	(auto&& c)
	{
		return std::isalpha(c,locale);
	});
}


template<class T>
bool isalpha(const T &val)
{
	return isalpha(val.begin(),val.end());
}


template<class T>
bool isnumeric(const T &val)
{
	return isnumeric(val.begin(),val.end());
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


inline
void tokens(const std::string &str,
            const char *const &sep,
            const std::function<void (const std::string &)> &func)
{
	using delim = boost::char_separator<char>;

	const delim d(sep);
	const boost::tokenizer<delim> tk(str,d);
	std::for_each(tk.begin(),tk.end(),func);
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


template<class It>
std::string detok(const It &begin,
                  const It &end,
                  const std::string &sep = " ")
{
	std::ostringstream str;
	std::for_each(begin,end,[&str,sep]
	(const auto &val)
	{
		str << val << sep;
	});

	return str.str().substr(0,ssize_t(str.tellp()) - sep.size());
}


inline
bool is_arg(const std::string &token,
            const std::string &keyed = "--")
{
	return token.size() > keyed.size() && token.find(keyed) == 0;
}


template<class It>
void parse_args(const It &begin,
                const It &end,
                const std::string &keyed,   //  = "--",
                const std::string &valued,  //  = "=",
                const std::function<void (const std::pair<std::string,std::string> &kv)> &func)
{
	std::for_each(begin,end,[&,keyed,valued,func]
	(const auto &token)
	{
		if(is_arg(token,keyed))
			func(split(token.substr(keyed.size()),valued));
	});
}


inline
void parse_args(const std::string &str,
                const std::string &keyed,   //  = "--"
                const std::string &valued,  //  = "="
                const std::string &toksep,  //  = " "
                const std::function<void (const std::pair<std::string,std::string> &kv)> &func)
{
	tokens(str,toksep.c_str(),[&,keyed,valued,func]
	(const std::string &token)
	{
		if(is_arg(token,keyed))
			func(split(token.substr(keyed.size()),valued));
	});
}


inline
std::string strip_args(const std::string &str,
                       const std::string &keyed = "--",
                       const std::string &toksep = " ")
{
	std::stringstream ret;
	tokens(str,toksep.c_str(),[&ret,&keyed,&toksep]
	(const auto &token)
	{
		if(!is_arg(token,keyed))
			ret << token << toksep;
	});

	return ret.str().substr(0,ssize_t(ret.tellp()) - toksep.size());
}


inline
std::string packetize(std::string &&str,
                      const size_t &max = 390)
{
	for(size_t i = 0, j = 0; i < str.size(); i++, j++)
	{
		if(j > max)
			str.insert(i,1,'\n');

		if(str[i] == '\n')
			j = 0;
	}

	return std::move(str);
}


inline
std::string packetize(const std::string &str,
                      const size_t &max = 390)
{
	std::string ret(str);
	return packetize(std::move(ret),max);
}


template<size_t SIZE>
const char *tolower(char (&buf)[SIZE])
{
	const size_t len = strnlen(buf,SIZE);
	std::transform(buf,buf+len,buf,[]
	(const char &c)
	{
		return std::tolower(c,locale);
	});

	return buf;
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


template<int CODE_FOR_SUCCESS = 0,
         class Exception = Internal,
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
		throw Exception(errc,"libircclient: (") << errc << ") " << irc_strerror(errc);
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
