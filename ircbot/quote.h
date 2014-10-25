/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Quote
{
	SendQ &sendq;
	std::string cmd;

  public:
	using flush_t = Stream::flush_t;
	static constexpr flush_t flush = Stream::flush;

	auto &get_sendq() const                         { return sendq;              }
	auto &get_cmd() const                           { return cmd;                }
	bool has_cmd() const                            { return !cmd.empty();       }

  protected:
	auto &get_sendq()                               { return sendq;              }
	auto &get_cmd()                                 { return cmd;                }

  public:
	// Send to server
	Quote &operator<<(const flush_t);
	Quote &operator()(const std::string &str);      // flush automatically
	Quote &operator()();                            // flush automatically

	// Append to stream
	template<class T> Quote &operator<<(const T &t);

	Quote(SendQ &sendq, const std::string &cmd = "");
};


inline
Quote::Quote(SendQ &sendq,
             const std::string &cmd):
sendq(sendq),
cmd(cmd)
{

}


inline
Quote &Quote::operator()()
{
	sendq << Stream::flush;
	return *this;
}


inline
Quote &Quote::operator()(const std::string &str)
{
	if(has_cmd())
		sendq << cmd << " ";

	sendq << str << Stream::flush;
	return *this;
}


template<class T>
Quote &Quote::operator<<(const T &t)
{
	if(has_cmd() && !sendq.has_pending())
		sendq << cmd << " ";

	sendq << t;
	return *this;
}


inline
Quote &Quote::operator<<(const flush_t)
{
	sendq << flush;
	return *this;
}
