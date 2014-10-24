/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Stream
{
	std::string target;
	std::ostringstream sendq;

  public:
	static constexpr struct flush_t {} flush {};        // Stream is terminated and sent

	auto &get_target() const                            { return target;                        }
	auto &get_sendq() const                             { return sendq;                         }
	auto get_str() const                                { return sendq.str();                   }
	auto has_target() const                             { return !target.empty();               }
	auto has_sendq() const                              { return get_str().empty();             }

  protected:
	auto &get_sendq()                                   { return sendq;                         }

  public:
	void set_target(const std::string &target)          { this->target = target;                }
	void reset();                                       // clear and begin stream with target
	void clear();                                       // clear all in sendq

	virtual Stream &operator<<(const flush_t) = 0;
	template<class T> Stream &operator<<(const T &t);   // Append data to sendq stream

 	// Manual semantics must be defined due to oversight in current gnulibstdc++
 	// std::ostringstream has incomplete semantics...
	Stream(const std::string &target);
	Stream(Stream &&other) noexcept;
	Stream(const Stream &other);
	Stream &operator=(Stream &&other) noexcept;
	Stream &operator=(const Stream &other);
	virtual ~Stream() = default;
};


inline
Stream::Stream(const std::string &target):
target(target)
{
	sendq.exceptions(std::ios_base::badbit|std::ios_base::failbit);
}


inline
Stream::Stream(const Stream &o):
target(o.target),
sendq(o.sendq.str())
{
	sendq.exceptions(std::ios_base::badbit|std::ios_base::failbit);
	sendq.setstate(o.sendq.rdstate());
	sendq.seekp(0,std::ios_base::end);
}


inline
Stream::Stream(Stream &&o)
noexcept try:
target(std::move(o.target)),
sendq(o.sendq.str())
{
	sendq.exceptions(std::ios_base::badbit|std::ios_base::failbit);
	sendq.setstate(o.sendq.rdstate());
	sendq.seekp(0,std::ios_base::end);
}
catch(const std::exception &e)
{
	// shouldn't happen, so terminate
	std::cerr << "Stream sendq move ctor: " << e.what() << std::endl;
}


inline
Stream &Stream::operator=(const Stream &o)
{
	target = o.target;
	sendq.clear();
	sendq.str(o.sendq.str());
	sendq.setstate(o.sendq.rdstate());
	sendq.seekp(0,std::ios_base::end);
	return *this;
}


inline
Stream &Stream::operator=(Stream &&o)
noexcept try
{
	target = std::move(o.target);
	sendq.clear();
	sendq.str(o.sendq.str());
	sendq.setstate(o.sendq.rdstate());
	sendq.seekp(0,std::ios_base::end);
	return *this;
}
catch(const std::exception &e)
{
	// shouldn't happen, don't bother recovering though
	std::cerr << "Stream sendq operator=&&: " << e.what() << std::endl;
	throw;
}


template<class T>
Stream &Stream::operator<<(const T &t)
{
	sendq << t;
	return *this;
}


inline
void Stream::clear()
{
	sendq.clear();
	sendq.str(std::string());
}


inline
void Stream::reset()
{
	if(has_target())
		sendq << get_target() << " ";
}
