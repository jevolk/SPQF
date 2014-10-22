/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Service : public Locutor
{
	Adb &adb;
	std::list<std::string> capture;                    // State of the current capture
	std::deque<std::string> queue;                     // Queue of terminators

	void next();                                       // Discard capture, move to next in queue

  public:
	auto &get_adb() const                              { return adb;                                   }
	auto queue_size() const                            { return queue.size();                          }
	auto capture_size() const                          { return capture.size();                        }
	virtual bool enabled() const                       { return get_ident().get<bool>("services");     }

  protected:
	using Capture = decltype(capture);

	auto &get_adb()                                    { return adb;                                   }
	auto &get_terminator() const                       { return queue.front();                         }

	// Passes a complete multipart message to subclass
	// once the handler here receives a terminator
	virtual void captured(const Capture &capture) = 0;

  public:
	void clear_capture()                               { capture.clear();                              }
	void clear_queue()                                 { queue.clear();                                }

	// [SEND] Add expected terminator every send
	void next_terminator(const std::string &str)       { queue.emplace_back(str);                      }
	void null_terminator()                             { queue.emplace_back(std::string());            }

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

	static const auto err =
	{
		"you are not authorized to perform this operation",
		"is not registered",
		"invalid parameters",
	};

	const auto &text = tolower(decolor(msg[TEXT]));
	if(std::any_of(err.begin(),err.end(),[&](auto&& t) { return text == t; }))
	{
		next();
		return;
	}

	const auto &term = tolower(queue.front());
	if(text.find(term) != std::string::npos)
	{
		const scope r(std::bind(&Service::next,this));
		captured(capture);
		return;
	}

	capture.emplace_back(decolor(msg[TEXT]));
}
catch(const std::out_of_range &e)
{
	throw Exception("Range error in Service::handle");
}


inline
void Service::next()
{
	queue.pop_front();
	capture.clear();
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
