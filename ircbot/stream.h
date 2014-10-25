/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Stream
{
	static thread_local std::ostringstream sbuf;        // bot.cpp

  public:
	static constexpr struct flush_t {} flush {};        // Stream is terminated and sent

	auto &get_sbuf() const                              { return sbuf;                         }
	auto has_sbuf() const                               { return !get_sbuf().str().empty();    }
	auto get_str() const                                { return get_sbuf().str();             }

  protected:
	auto &get_sbuf()                                    { return sbuf;                         }

  public:
	void clear();                                       // clear all in sbuf

	virtual Stream &operator<<(const flush_t) = 0;
	Stream &operator()(const std::string &str);         // flush automatically
	Stream &operator()();                               // flush automatically

	template<class T> Stream &operator<<(const T &t);   // Append data to sbuf stream

	Stream();
	virtual ~Stream() = default;
};


inline
Stream::Stream(void)
{
	sbuf.exceptions(std::ios_base::badbit|std::ios_base::failbit);

}


template<class T>
Stream &Stream::operator<<(const T &t)
{
	sbuf << t;
	return *this;
}


inline
Stream &Stream::operator()()
{
	return operator<<(flush);
}


inline
Stream &Stream::operator()(const std::string &str)
{
	operator<<(str);
	return operator<<(flush);
}


inline
void Stream::clear()
{
	sbuf.clear();
	sbuf.str(std::string());
}
