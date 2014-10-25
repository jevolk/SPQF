/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class SendQ
{
	irc_session_t *sess;
	std::ostringstream sendq;

  protected:
	auto &get_sess()               { return sess;                 }

	template<class Func, class... Args> bool call(std::nothrow_t, Func&& func, Args&&... args);
	template<class Func, class... Args> void call(Func&& func, Args&&... args);

	template<class... VA_LIST> void quote(const char *const &fmt, VA_LIST&&... ap);
	void quote(const std::string &str);

  public:
	using flush_t = Stream::flush_t;
	static constexpr flush_t flush {};

	auto &get_sess() const         { return sess;                 }
	auto has_pending() const       { return !sendq.str().empty(); }

    // Send to server
	SendQ &operator<<(const flush_t);

	// Append to pending stream
	template<class T> SendQ &operator<<(const T &t);

	SendQ(irc_session_t *const &sess);
};


inline
SendQ::SendQ(irc_session_t *const &sess):
sess(sess)
{

}


template<class T>
SendQ &SendQ::operator<<(const T &t)
{
	sendq << t;
	return *this;
}


inline
SendQ &SendQ::operator<<(const flush_t)
{
	const scope _([&]
	{
		sendq.clear();
		sendq.str(std::string());
	});

	quote(sendq.str());
	return *this;
}


inline
void SendQ::quote(const std::string &str)
{
	call(irc_send_raw,"%s",str.c_str());
}


template<class... VA_LIST>
void SendQ::quote(const char *const &fmt,
                  VA_LIST&&... ap)
{
	call(irc_send_raw,fmt,std::forward<VA_LIST>(ap)...);
}


template<class Func,
         class... Args>
void SendQ::call(Func&& func,
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
bool SendQ::call(std::nothrow_t,
                 Func&& func,
                 Args&&... args)
{
	return irc_call(std::nothrow,get_sess(),func,std::forward<Args>(args)...);
}
