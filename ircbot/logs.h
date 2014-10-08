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
		const std::string &acct;
		const std::string &nick;
		const std::string &msg;
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

		bool operator()(const ClosureArgs &args) const override;
	};

	using Closure = std::function<void (const ClosureArgs &)>;

	void for_each(const std::string &path, const Closure &closure) const;
	void for_each(const Chan &chan, const Closure &closure) const;

	void for_each(const std::string &path, const Filter &filter, const Closure &closure) const;
	void for_each(const Chan &chan, const Filter &filter, const Closure &closure) const;

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
void Logs::for_each(const Chan &chan,
                    const Filter &filter,
                    const Closure &closure)
const
{
	const Log &clog = chan.get_log();
	const std::string &path = clog.get_path();
	for_each(path,filter,closure);
}


inline
void Logs::for_each(const Chan &chan,
                    const Closure &closure)
const
{
	const Log &clog = chan.get_log();
	const std::string &path = clog.get_path();
	for_each(path,closure);
}


inline
void Logs::for_each(const std::string &path,
                    const Filter &filter,
                    const Closure &closure)
const
{
	for_each(path,[&filter,&closure]
	(const ClosureArgs &a)
	{
		if(filter(a))
			closure(a);
	});
}


inline
void Logs::for_each(const std::string &path,
                    const Closure &closure)
const try
{
	std::ifstream file;
	file.exceptions(std::ios_base::badbit);
	file.open(path,std::ios_base::in);

	while(1)
	{
		std::string buf;
		std::getline(file,buf);
		if(file.fail() || file.eof())
			break;

		const size_t t_a_sep = buf.find(' ');
		const size_t a_n_sep = buf.find(' ',t_a_sep+1);
		const size_t n_m_sep = buf.find(' ',a_n_sep+1);

		const time_t &time = boost::lexical_cast<time_t>(buf.substr(0,t_a_sep));
		const std::string &acct = buf.substr(t_a_sep+1,a_n_sep - t_a_sep - 1);
		const std::string &nick = buf.substr(a_n_sep+1,n_m_sep - a_n_sep - 1);
		const std::string &msg = buf.substr(n_m_sep+1);

		const ClosureArgs args {time,acct,nick,msg};
		closure(args);
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
