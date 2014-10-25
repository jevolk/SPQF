/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Cmds
{
	Sess &sess;

  public:
	// [SEND] Compositions
	template<class It> void accept_del(const It &begin, const It &end);
	template<class It> void accept_add(const It &begin, const It &end);

	template<class It> void monitor_add(const It &begin, const It &end);
	template<class It> void monitor_del(const It &begin, const It &end);

	void monitor_status()      { Quote(sess,"MONITOR S");  }
	void monitor_clear()       { Quote(sess,"MONITOR C");  }
	void monitor_list()        { Quote(sess,"MONITOR L");  }

	template<class It> void topics(const It &begin, const It &end, const std::string &server = "");
	template<class It> void isons(const It &begin, const It &end);

	Cmds(Sess &sess): sess(sess) {}
};


template<class It>
void Cmds::isons(const It &begin,
                 const It &end)
{
	Quote ison(sess,"ISON");

	ison << ":";
	std::for_each(begin,end,[&]
	(const auto &s)
	{
		ison << s << " ";
	});
}


template<class It>
void Cmds::topics(const It &begin,
                  const It &end,
                  const std::string &server)
{
	Quote list(sess,"LIST");

	std::for_each(begin,end,[&]
	(const auto &s)
	{
		list << s << ",";
	});

	if(!server.empty())
		list << " " << server;
}


template<class It>
void Cmds::accept_add(const It &begin,
                      const It &end)
{
	Quote accept(sess,"ACCEPT");

	std::for_each(begin,end,[&]
	(const auto &s)
	{
		accept << s << ",";
	});
}


template<class It>
void Cmds::accept_del(const It &begin,
                      const It &end)
{
	Quote accept(sess,"ACCEPT");

	std::for_each(begin,end,[&]
	(const auto &s)
	{
		accept << "-" << s << ",";
	});
}


template<class It>
void Cmds::monitor_add(const It &begin,
                       const It &end)
{
	Quote monitor(sess,"MONITOR");

	monitor << "+ ";
	std::for_each(begin,end,[&]
	(const auto &s)
	{
		monitor << s << ",";
	});
}


template<class It>
void Cmds::monitor_del(const It &begin,
                       const It &end)
{
	Quote monitor(sess,"MONITOR");

	monitor << "+ ";
	std::for_each(begin,end,[&]
	(const auto &s)
	{
		monitor << s << ",";
	});
}
