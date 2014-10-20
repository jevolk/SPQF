/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Service : public Locutor
{
	Adb &adb;
	std::list<std::string> capture;                // State of the current capture
	std::deque<std::string> queue;                 // Queue of terminators

  public:
	auto &get_adb() const                          { return adb;                          }
	auto queue_size() const                        { return queue.size();                 }
	auto capture_size() const                      { return capture.size();               }

  protected:
	using Capture = decltype(capture);

	auto &get_adb()                                { return adb;                          }
	auto &get_terminator() const                   { return queue.front();                }

	// Passes a complete multipart message to subclass once terminated
	virtual void captured(const Capture &capture) = 0;

  public:
	void clear_capture()                           { capture.clear();                     }
	void clear_queue()                             { queue.clear();                       }

	// [SEND] Add expected terminator every send
	void next_terminator(const std::string &str)   { queue.emplace_back(str);             }
	void null_terminator()                         { queue.emplace_back(std::string());   }

	// [RECV] Called by Bot handlers
	void handle(const Msg &msg);

	Service(Adb &adb, Sess &sess, const std::string &name):
	        Locutor(sess,name), adb(adb) {}

	friend std::ostream &operator<<(std::ostream &s, const Service &srv);
};


inline
void Service::handle(const Msg &msg)
try
{
	using namespace fmt::NOTICE;

	if(queue.empty())
		return;

	if(msg.get_name() != "NOTICE")
		throw Exception("Service handler only reads NOTICE.");

	static const auto &term_err = "You are not authorized to perform this operation.";
	const auto &text = tolower(decolor(msg[TEXT]));
	const auto &term = tolower(queue.front());
	const auto reset = [&]
	{
		queue.pop_front();
		capture.clear();
	};

	if(text.find(term_err) != std::string::npos)
	{
		reset();
		return;
	}
	else if(text.find(term) != std::string::npos)
	{
		const scope r(reset);
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
