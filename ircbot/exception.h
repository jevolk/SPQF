/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


/**
 * Exception base. Used directly when interrupting a thread, possibly to exit.
 */
class Interrupted : public std::runtime_error
{
	int c;

  public:
	const int &code() const       { return c;      }
	operator std::string() const  { return what(); }

	Interrupted(const int &c, const std::string &what = ""): std::runtime_error(what), c(c) {}
	Interrupted(const int &c, std::string &&what): std::runtime_error(std::move(what)), c(c) {}
	Interrupted(const std::string &what = ""): std::runtime_error(what), c(0) {}
	Interrupted(std::string &&what): std::runtime_error(std::move(what)), c(0) {}

	template<class T> Interrupted operator<<(const T &t) const &&
	{
		return {code(),static_cast<std::stringstream &>(std::stringstream() << what() << t).str()};
	}

	friend std::ostream &operator<<(std::ostream &s, const Interrupted &e)
	{
		return (s << e.what());
	}
};


/**
 * Internal errors hidden from the interlocutor and passed straight through to the console.
 */
struct Internal : public Interrupted
{
	template<class... Args> Internal(Args&&... args): Interrupted(std::forward<Args&&>(args)...) {}

	template<class T> Internal operator<<(const T &t) const &&
	{
		return {code(),static_cast<std::stringstream &>(std::stringstream() << what() << t).str()};
	}
};


/**
 * General errors reported to the interlocutor, and usually not rethrown to the console.
 */
struct Exception : public Internal
{
	template<class... Args> Exception(Args&&... args): Internal(std::forward<Args&&>(args)...) {}

	template<class T> Exception operator<<(const T &t) const &&
	{
		return {code(),static_cast<std::stringstream &>(std::stringstream() << what() << t).str()};
	}
};


/**
 * Run-time assertions that are reported to the interlocutor, possibly with
 * the prefix "internal error:" and then the console. They should not be purposely
 * triggered, though the result is seen so the interlocutor knows of the failure.
 */
struct Assertive : public Exception
{
	template<class... Args> Assertive(Args&&... args): Exception(std::forward<Args&&>(args)...) {}

	template<class T> Assertive operator<<(const T &t) const &&
	{
		return {code(),static_cast<std::stringstream &>(std::stringstream() << what() << t).str()};
	}
};
