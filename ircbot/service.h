/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Service : public Stream
{
	Adb &adb;
	Sess &sess;
	std::string name;
	std::list<std::string> capture;                    // State of the current capture
	std::deque<std::forward_list<std::string>> queue;  // Queue of terminators

	void next();                                       // Discard capture, move to next in queue

  public:
	auto &get_adb() const                              { return adb;                                   }
	auto &get_sess() const                             { return sess;                                  }
	auto &get_opts() const                             { return get_sess().get_opts();                 }
	auto get_name() const                              { return name;                                  }
	auto queue_size() const                            { return queue.size();                          }
	auto capture_size() const                          { return capture.size();                        }
	virtual bool enabled() const                       { return get_opts().get<bool>("services");      }

  protected:
	using Capture = decltype(capture);

	auto &get_adb()                                    { return adb;                                   }
	auto &get_sess()                                   { return sess;                                  }
	auto &get_terminator() const                       { return queue.front();                         }

	// Passes a complete multipart message to subclass
	// once the handler here receives a terminator
	virtual void captured(const Capture &capture) = 0;

  public:
	void clear_capture()                               { capture.clear();                              }
	void clear_queue()                                 { queue.clear();                                }

	// [SEND] Add expected terminator every send
	void terminator_next(const std::string &str)       { queue.push_back({tolower(str)});              }
	void terminator_also(const std::string &str)       { queue.back().emplace_front(tolower(str));     }
	void terminator_errors()                           { queue.push_back({"",""});                     }
	void terminator_any()                              { queue.push_back({""});                        }

	// [RECV] Called by Bot handlers
	void handle(const Msg &msg);

	template<class... Args>
	Service(Adb &adb, Sess &sess, const std::string &name):
	        adb(adb), sess(sess), name(name) {}

	friend std::ostream &operator<<(std::ostream &s, const Service &srv);
};


inline
void Service::handle(const Msg &msg)
try
{
	using namespace fmt::NOTICE;

	static const auto errors =
	{
		"you are not authorized",
		"invalid parameters",
		"is not registered",
		"is not online",
	};

	static const auto ignores =
	{
		"this nickname is registered",
	};

	if(queue.empty())
		return;

	if(msg.get_name() != "NOTICE")
		throw Exception("Service handler only reads NOTICE.");

	const auto &term = queue.front();
	const auto &text = tolower(decolor(msg[TEXT]));

	const size_t terms = std::distance(term.begin(),term.end());
	const bool any_term = terms == 1 && term.front().empty();
	const bool err_term = terms == 2 && std::all_of(term.begin(),term.end(),[](auto&& t) { return t.empty(); });
	const bool match = std::any_of(term.begin(),term.end(),[&](auto&& t) { return text.find(t) != std::string::npos; });
	const bool error = std::any_of(errors.begin(),errors.end(),[&](auto&& t) { return text == t; });
	const bool ignore = std::any_of(ignores.begin(),ignores.end(),[&](auto&& t) { return text == t; });

	if(match)
	{
		const scope r(std::bind(&Service::next,this));
		captured(capture);
		return;
	}

	if(ignore)
		return;

	if(err_term && !error)
	{
		queue.pop_front();
		handle(msg);
		return;
	}

	if(any_term || error)
	{
		next();
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
	s << "Name:             " << srv.get_name() << std::endl;
	s << "Queue size:       " << srv.queue_size() << std::endl;
	s << "Capture size:     " << srv.capture_size() << std::endl;
	return s;
}
