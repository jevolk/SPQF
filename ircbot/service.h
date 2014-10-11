/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Service : public Locutor
{
	Adb &adb;
	std::list<std::string> capture;               // State of the current capture
	std::deque<std::string> queue;                // Queue of terminators

  public:
	const Adb &get_adb() const                     { return adb;                    }
	size_t queue_size() const                      { return queue.size();           }
	size_t capture_size() const                    { return capture.size();         }

  protected:
	using Capture = decltype(capture);

	Adb &get_adb()                                 { return adb;                    }
	const std::string &get_terminator() const      { return queue.front();          }

	// Add expected terminator every send
	void next_terminator(const std::string &str)   { queue.emplace_back(str);       }

	// Passes a complete multipart message to subclass once terminated
	virtual void captured(const Capture &capture) = 0;

  public:
	void handle(const Msg &msg);

	Service(Adb &adb, Sess &sess, const std::string &name);
	virtual ~Service() = default;

	friend std::ostream &operator<<(std::ostream &s, const Service &srv);
};


inline
Service::Service(Adb &adb,
                 Sess &sess,
                 const std::string &name):
Locutor(sess,name),
adb(adb)
{


}


inline
void Service::handle(const Msg &msg)
try
{
	using namespace fmt::NOTICE;

	if(queue.empty())
		return;

	if(msg.get_name() != "NOTICE")
		throw Exception("Service handler only reads NOTICE.");

	if(decolor(msg[TEXT]) == queue.front())
	{
		const scope reset([&]
		{
			queue.pop_front();
			capture.clear();
		});

		captured(capture);
	}
	else capture.emplace_back(decolor(msg[TEXT]));
}
catch(const std::out_of_range &e)
{
	throw Exception("Range error in Service::handle");
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Service &srv)
{
	s << "[Service]:        " << std::endl;
	s << "Name:             " << srv.get_target() << std::endl;
	s << "Queue size:       " << srv.queue_size() << std::endl;
	s << "Capture size:     " << srv.capture_size() << std::endl;
	return s;
}
