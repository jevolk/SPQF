/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Quote : public Stream
{
	irc_session_t *sess;

  protected:
	auto &get_sess()            { return sess;     }

	template<class Func, class... Args> bool call(std::nothrow_t, Func&& func, Args&&... args);
	template<class Func, class... Args> void call(Func&& func, Args&&... args);

	template<class... VA_LIST> void quote(const char *const &fmt, VA_LIST&&... ap);
	void quote(const std::string &str);

  public:
	auto &get_sess() const      { return sess;     }

	// Send to server
	Quote &operator<<(const flush_t f) override;

	// Append to stream after cmd prefix (if one exists)
	template<class T> Quote &operator<<(const T &t);
	Quote &operator()();                               // flush automatically
	Quote &operator()(const std::string &str);         // flush automatically

	Quote(irc_session_t *const &sess, const std::string &cmd = "");
};


inline
Quote::Quote(irc_session_t *const &sess,
             const std::string &cmd):
Stream(cmd),
sess(sess)
{
	Stream::reset();
}


inline
Quote &Quote::operator()(const std::string &str)
{
	operator<<(str);
	return operator()();
}


inline
Quote &Quote::operator()()
{
	operator<<(flush);
	return *this;
}


template<class T>
Quote &Quote::operator<<(const T &t)
{
	Stream::operator<<(t);
	return *this;
}


inline
Quote &Quote::operator<<(const flush_t)
{
	const scope r([&]
	{
		clear();
		reset();
	});

	quote(get_str());
	return *this;
}


inline
void Quote::quote(const std::string &str)
{
	call(irc_send_raw,"%s",str.c_str());
}


template<class... VA_LIST>
void Quote::quote(const char *const &fmt,
                  VA_LIST&&... ap)
{
	call(irc_send_raw,fmt,std::forward<VA_LIST>(ap)...);
}


template<class Func,
         class... Args>
void Quote::call(Func&& func,
                 Args&&... args)
{
	static const uint32_t CALL_MAX_ATTEMPTS = 25;
	static constexpr std::chrono::milliseconds CALL_THROTTLE {750};

	// Loop to reattempt for certain library errors
	for(size_t i = 0; i < CALL_MAX_ATTEMPTS; i++) try
	{
		irc_call(get_sess(),func,std::forward<Args>(args)...);
		return;
	}
	catch(const Exception &e)
	{
		// No more reattempts at the limit
		if(i >= CALL_MAX_ATTEMPTS - 1)
			throw;

		switch(e.code())
		{
			case LIBIRC_ERR_NOMEM:
				// Sent too much data without letting libirc recv(), we can throttle and try again.
				std::cerr << "call(): \033[1;33mthrottling send\033[0m"
				          << " (attempt: " << i << " timeout: " << CALL_THROTTLE.count() << ")"
				          << std::endl;

				std::this_thread::sleep_for(CALL_THROTTLE);
				continue;

			default:
				// No reattempt for everything else
				throw;
		}
	}
}


template<class Func,
         class... Args>
bool Quote::call(std::nothrow_t,
                 Func&& func,
                 Args&&... args)
{
	return irc_call(std::nothrow,get_sess(),func,std::forward<Args>(args)...);
}
