/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


/**
 * Internal errors hidden from the interlocutor and passed straight through to the console.
 */
class Internal : public std::runtime_error
{
	int c;

  public:
	const int &code() const       { return c;      }
	operator std::string() const  { return what(); }

	Internal(const int &c, const std::string &what = ""): std::runtime_error(what), c(c) {}
	Internal(const std::string &what = ""): std::runtime_error(what), c(0) {}

	template<class T> Internal operator<<(const T &t) const &&
	{
		return {code(),static_cast<std::stringstream &>(std::stringstream() << what() << t).str()};
	}

	friend std::ostream &operator<<(std::ostream &s, const Internal &e)
	{
		return (s << e.what());
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
