/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */

namespace irc {
namespace log {


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
struct FilterAny : Filter,
                   std::vector<F>
{
	bool operator()(const ClosureArgs &args) const override;

	template<class... A> FilterAny(A&&... a): std::vector<F>{std::forward<A>(a)...} {}
};

template<class F>
struct FilterAll : Filter,
                   std::vector<F>
{

	bool operator()(const ClosureArgs &args) const override;
	template<class... A> FilterAll(A&&... a): std::vector<F>{std::forward<A>(a)...} {}
};

template<class F>
struct FilterNone : Filter,
                    std::vector<F>
{
	bool operator()(const ClosureArgs &args) const override;

	template<class... A> FilterNone(A&&... a): std::vector<F>{std::forward<A>(a)...} {}
};

struct SimpleFilter : Filter
{
	enum { START = 0, END = 1 };

	std::pair<time_t,time_t> time {0,0};
	std::string acct;
	std::string nick;
	std::string type;

	bool operator()(const ClosureArgs &args) const override;
};


class Log
{
	std::string path;
	std::ofstream file;

  public:
	enum Field
	{
		VERS        = 0,     // Version/format of this line
		TIME        = 1,     // Epoch time in seconds
		ACCT        = 2,     // NickServ account name
		NICK        = 3,     // Nickname
		TYPE        = 4,     // Event type

		_NUM_FIELDS
	};

	auto &get_path() const                               { return path;             }
	auto &get_file() const                               { return file;             }

	void operator()(const Msg &msg, const Chan &chan, const User &user);
	void flush();

	Log(const std::string &path);
	~Log() noexcept;
};


class Logs
{
	Sess &sess;
	Chans &chans;
	Users &users;
	const Opts &opts;

	std::string get_path(const std::string &name) const;

  public:
	using Closure = std::function<bool (const ClosureArgs &)>;

	// Reading 
	// returns false if break early - "remain true to the end"
	bool for_each(const std::string &name, const Closure &closure) const;
	bool for_each(const std::string &name, const Filter &filter, const Closure &closure) const;
	size_t count(const std::string &name, const Filter &filter) const;
	bool atleast(const std::string &name, const Filter &filter, const size_t &count) const;
	bool exists(const std::string &name, const Filter &filter) const;

	// Writing
	bool log(const Msg &msg, const Chan &chan, const User &user);

	Logs(Sess &sess, Chans &chans, Users &users);
};


} // namespace log
} // namespace irc



template<class F>
bool irc::log::FilterAny<F>::operator()(const ClosureArgs &args)
const
{
	return std::any_of(this->begin(),this->end(),[&args]
	(const F &filter)
	{
		return filter(args);
	});
}


template<class F>
bool irc::log::FilterAll<F>::operator()(const ClosureArgs &args)
const
{
	return std::all_of(this->begin(),this->end(),[&args]
	(const F &filter)
	{
		return filter(args);
	});
}


template<class F>
bool irc::log::FilterNone<F>::operator()(const ClosureArgs &args)
const
{
	return std::none_of(this->begin(),this->end(),[&args]
	(const F &filter)
	{
		return filter(args);
	});
}
