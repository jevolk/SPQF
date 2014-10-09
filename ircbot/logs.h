/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Logs
{
	Chans &chans;
	Users &users;
	std::mutex &bot;

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

	// returns false if break early - "remain true to the end"
	bool for_each(const std::string &path, const Closure &closure) const;
	bool for_each(const std::string &path, const Filter &filter, const Closure &closure) const;
	size_t count(const std::string &path, const Filter &filter) const;
	bool exists(const std::string &path, const Filter &filter) const;

	bool for_each(const Chan &chan, const Closure &closure) const;
	bool for_each(const Chan &chan, const Filter &filter, const Closure &closure) const;
	size_t count(const Chan &chan, const Filter &filter) const;
	bool exists(const Chan &chan, const Filter &filter) const;

	Logs(Chans &chans, Users &users, std::mutex &bot);

	friend std::ostream &operator<<(std::ostream &s, const Logs &logs);
};


inline
Logs::Logs(Chans &chans,
           Users &users,
           std::mutex &bot):
chans(chans),
users(users),
bot(bot)
{


}


inline
bool Logs::exists(const Chan &chan,
                  const Filter &filter)
const
{
	const Log &clog = chan.get_log();
	const std::string &path = clog.get_path();
	return exists(path,filter);
}


inline
size_t Logs::count(const Chan &chan,
                   const Filter &filter)
const
{
	const Log &clog = chan.get_log();
	const std::string &path = clog.get_path();
	return count(path,filter);
}


inline
bool Logs::exists(const std::string &path,
                  const Filter &filter)
const
{
	return !for_each(path,[&filter]
	(const ClosureArgs &a) -> bool
	{
		return !filter(a);
	});
}


inline
size_t Logs::count(const std::string &path,
                    const Filter &filter)
const
{
	size_t ret = 0;
	for_each(path,[&filter,&ret]
	(const ClosureArgs &a) -> bool
	{
		ret += filter(a);
		return true;
	});

	return ret;
}


inline
bool Logs::for_each(const Chan &chan,
                    const Filter &filter,
                    const Closure &closure)
const
{
	const Log &clog = chan.get_log();
	const std::string &path = clog.get_path();
	return for_each(path,filter,closure);
}


inline
bool Logs::for_each(const Chan &chan,
                    const Closure &closure)
const
{
	const Log &clog = chan.get_log();
	const std::string &path = clog.get_path();
	return for_each(path,closure);
}


inline
bool Logs::for_each(const std::string &path,
                    const Filter &filter,
                    const Closure &closure)
const
{
	return for_each(path,[&filter,&closure]
	(const ClosureArgs &a)
	{
		if(!filter(a))
			return true;

		return closure(a);
	});
}


inline
bool Logs::for_each(const std::string &path,
                    const Closure &closure)
const try
{
	std::ifstream file;
	file.exceptions(std::ios_base::badbit);
	file.open(path,std::ios_base::in);

	while(1)
	{
		char buf[64] alignas(16);
		file.getline(buf,sizeof(buf));
		const size_t len = strlen(buf);
		if(!len)
			return true;

		std::array<const char *, Log::_NUM_FIELDS> field;
		std::replace(buf,buf+len,' ','\0');

		size_t i = 0;
		const char *ptr = buf, *const end = buf + len;
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
std::ostream &operator<<(std::ostream &s,
                         const Logs &logs)
{

	return s;
}
