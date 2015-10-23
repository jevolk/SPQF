/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Logs
{
	Sess &sess;
	Chans &chans;
	Users &users;
	const Opts &opts;

	std::string get_path(const std::string &name) const;

  public:
	struct ClosureArgs
	{
		const time_t &time;
		const char *const &acct;
		const char *const &nick;
		const char *const &type;
	};

	struct Filter
	{
		virtual bool operator()(const ClosureArgs &args) const = 0;
	};

	template<class F>
	struct FilterAny : public Filter,
	                   public std::vector<F>
	{
		bool operator()(const ClosureArgs &args) const override;
		template<class... A> FilterAny(A&&... a): std::vector<F>{std::forward<A>(a)...} {}
	};

	template<class F>
	struct FilterAll : public Filter,
	                   public std::vector<F>
	{
		bool operator()(const ClosureArgs &args) const override;
		template<class... A> FilterAll(A&&... a): std::vector<F>{std::forward<A>(a)...} {}
	};

	template<class F>
	struct FilterNone : public Filter,
	                    public std::vector<F>
	{
		bool operator()(const ClosureArgs &args) const override;
		template<class... A> FilterNone(A&&... a): std::vector<F>{std::forward<A>(a)...} {}
	};

	struct SimpleFilter : public Filter
	{
		enum {START, END};

		std::pair<time_t,time_t> time {0,0};
		std::string acct;
		std::string nick;
		std::string type;

		bool operator()(const ClosureArgs &args) const override;
	};

	using Closure = std::function<bool (const ClosureArgs &)>;

	// Reading 
	// returns false if break early - "remain true to the end"
	bool for_each(const std::string &name, const Closure &closure) const;
	bool for_each(const std::string &name, const Filter &filter, const Closure &closure) const;
	size_t count(const std::string &name, const Filter &filter) const;
	bool atleast(const std::string &name, const Filter &filter, const size_t &count) const;
	bool exists(const std::string &name, const Filter &filter) const;

	bool log(const Msg &msg, const Chan &chan, const User &user);

	Logs(Sess &sess, Chans &chans, Users &users);
};


inline
Logs::Logs(Sess &sess,
           Chans &chans,
           Users &users):
sess(sess),
chans(chans),
users(users),
opts(sess.get_opts())
{
	if(!opts.get<bool>("logging"))
		return;

	const int ret = mkdir(opts["logdir"].c_str(),0777);
	if(ret && errno != EEXIST)
		throw Internal("Failed to create specified logfile directory [") << opts["logdir"] << "]";
}


inline
bool Logs::log(const Msg &msg,
               const Chan &chan,
               const User &user)
try
{
	if(!opts.get<bool>("logging"))
		return false;

	Log lf(get_path(chan.get_name()));
	lf(msg,chan,user);
	return true;
}
catch(const Internal &e)
{
	std::cerr << e << std::endl;
	return false;
}


inline
bool Logs::exists(const std::string &name,
                  const Filter &filter)
const
{
	return !for_each(name,[&filter]
	(const ClosureArgs &a) -> bool
	{
		return !filter(a);
	});
}


inline
bool Logs::atleast(const std::string &name,
                   const Filter &filter,
                   const size_t &count)
const
{
	size_t ret = 0;
	return !count || !for_each(name,[&filter,&ret,&count]
	(const ClosureArgs &a) -> bool
	{
		ret += filter(a);
		return ret < count;
	});
}


inline
size_t Logs::count(const std::string &name,
                   const Filter &filter)
const
{
	size_t ret = 0;
	for_each(name,[&filter,&ret]
	(const ClosureArgs &a) -> bool
	{
		ret += filter(a);
		return true;
	});

	return ret;
}


inline
bool Logs::for_each(const std::string &name,
                    const Filter &filter,
                    const Closure &closure)
const
{
	return for_each(name,[&filter,&closure]
	(const ClosureArgs &a)
	{
		if(!filter(a))
			return true;

		return closure(a);
	});
}


inline
bool Logs::for_each(const std::string &name,
                    const Closure &closure)
const try
{
	std::ifstream file;
	const auto path(get_path(name));
	file.exceptions(std::ios_base::badbit);
	file.open(path,std::ios_base::in);

	while(1)
	{
		char buf[64] alignas(16);
		file.getline(buf,sizeof(buf));
		const size_t len = strlen(buf);
		std::replace(buf,buf+len,' ','\0');
		if(!len)
			return true;

		size_t i = 0;
		const char *ptr = buf, *const end = buf + len;
		std::array<const char *, Log::_NUM_FIELDS> field;
		while(ptr < end)
		{
			field[i++] = ptr;
			ptr += strlen(ptr) + 1;
		}

		while(i < field.size())
			field[i++] = nullptr;

		const ClosureArgs args
		{
			atoll(field[Log::TIME]),
			field[Log::ACCT],
			field[Log::NICK],
			field[Log::TYPE],
		};

		if(!closure(args))
			return false;
	}
}
catch(const std::ios_base::failure &e)
{
	throw Exception(e.what());
}


inline
bool Logs::SimpleFilter::operator()(const ClosureArgs &args)
const
{
	if(std::get<START>(time) && args.time < std::get<START>(time))
		return false;

	if(std::get<END>(time) && args.time >= std::get<END>(time))
		return false;

	if(!acct.empty() && acct != args.acct)
		return false;

	if(!nick.empty() && nick != args.nick)
		return false;

	if(!type.empty() && type != args.type)
		return false;

	return true;
}



template<class F>
bool Logs::FilterAny<F>::operator()(const ClosureArgs &args)
const
{
	return std::any_of(this->begin(),this->end(),[&args]
	(const F &filter)
	{
		return filter(args);
	});
}



template<class F>
bool Logs::FilterAll<F>::operator()(const ClosureArgs &args)
const
{
	return std::all_of(this->begin(),this->end(),[&args]
	(const F &filter)
	{
		return filter(args);
	});
}



template<class F>
bool Logs::FilterNone<F>::operator()(const ClosureArgs &args)
const
{
	return std::none_of(this->begin(),this->end(),[&args]
	(const F &filter)
	{
		return filter(args);
	});
}


inline
std::string Logs::get_path(const std::string &name)
const
{
	return opts["logdir"] + "/" + name;
}
