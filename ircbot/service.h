/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Service : public Locutor
{
  protected:
	using Capture = std::list<std::string>;

  private:
	Adb &adb;
	Capture capture;                              // State of the current capture
	std::deque<std::string> queue;                // Queue of terminators

  protected:
	Adb &get_adb()                                 { return adb;                    }
	size_t queue_size() const                      { return queue.size();           }
	size_t capture_size() const                    { return capture.size();         }

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
		return;
	}

	capture.emplace_back(decolor(msg[TEXT]));
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
