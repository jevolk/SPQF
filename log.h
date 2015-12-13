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

using Closure = std::function<bool (const ClosureArgs &)>;


class Filter : protected std::vector<Closure>
{
  public:
	virtual bool operator()(const ClosureArgs &args) const = 0;

  protected:
	template<class... A> Filter(A&&... a): std::vector<Closure>{std::forward<A>(a)...} {}
};

class FilterAny : public Filter
{
	bool operator()(const ClosureArgs &args) const override;

  public:
	template<class... A> FilterAny(A&&... a): Filter(std::forward<A>(a)...) {}
};

class FilterAll : public Filter
{
	bool operator()(const ClosureArgs &args) const override;

  public:
	template<class... A> FilterAll(A&&... a): Filter(std::forward<A>(a)...) {}
};

class FilterNone : public Filter
{
	bool operator()(const ClosureArgs &args) const override;

  public:
	template<class... A> FilterNone(A&&... a): Filter(std::forward<A>(a)...) {}
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


std::string get_path(const std::string &name);

// Reading
// returns false if break early - "remain true to the end"
bool for_each(const std::string &name, const Closure &closure);
bool for_each(const std::string &name, const Filter &filter, const Closure &closure);
size_t count(const std::string &name, const Filter &filter);
bool atleast(const std::string &name, const Filter &filter, const size_t &count);
bool exists(const std::string &name, const Filter &filter);

// Writing
bool log(const Msg &msg, const Chan &chan, const User &user);
void init();


} // namespace log
} // namespace irc
