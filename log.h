/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


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


inline
Log::Log(const std::string &path)
try:
path(path)
{
	file.exceptions(std::ios_base::badbit|std::ios_base::failbit);
	file.open(path,std::ios_base::app);
}
catch(const std::exception &e)
{
	throw Internal("Failed opening Log [") << path << "]";
}


inline
Log::~Log()
noexcept try
{
	flush();
	file.close();
}
catch(const std::exception &e)
{
	std::cerr << "Failed closing Log [" << get_path() << "]" << std::endl;
	return;
}


inline
void Log::flush()
try
{
	file.flush();
}
catch(const std::ios_base::failure &f)
{
	throw Internal("Failed flushing log [") << get_path() << "]: " << f.what();
}


inline
void Log::operator()(const Msg &msg,
                     const Chan &chan,
                     const User &user)
try
{
	static const uint VERSION = 0;
	static time_t time;
	std::time(&time);

	const std::string &acct = user.is_logged_in()? user.get_acct() : "*";
	const std::string &nick = user.get_nick();
	const std::string &type = !msg.get_code()? msg.get_name().substr(0,3):
	                                           lex_cast(msg.get_code());
	file << VERSION
	     << ' '
	     << time
	     << ' '
	     << acct
	     << ' '
	     << nick
	     << ' '
	     << type
	     << '\n';
}
catch(const std::ios_base::failure &f)
{
	throw Internal("Failed appending log [") << get_path() << "]: " << f.what();
}
