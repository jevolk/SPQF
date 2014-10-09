/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Log
{
	const Sess &sess;
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

	const std::string &get_path() const                        { return path;             }

	void operator()(const User &user, const Msg &msg);
	auto flush() -> decltype(file.flush())                     { return file.flush();     }

	Log(const Sess &sess, const std::string &name);
};


inline
Log::Log(const Sess &sess,
         const std::string &name)
try:
sess(sess),
path([&]
{
	const Ident &id = sess.get_ident();
	const std::string &dir = id["logdir"];
	mkdir(dir.c_str(),0777);
	return dir + "/" + name;
}())
{
	file.exceptions(std::ios_base::badbit|std::ios_base::failbit);
	file.open(path,std::ios_base::app);
}
catch(const std::exception &e)
{
	std::cerr << "Failed opening Log [" << name << "]" << std::endl;
	throw;
}


inline
void Log::operator()(const User &user,
                     const Msg &msg)
try
{
	static const uint VERSION = 0;
	static time_t time;
	std::time(&time);

	const std::string &acct = user.is_logged_in()? user.get_acct() : "*";
	const std::string &nick = user.get_nick();
	const std::string &type = !msg.get_code()? msg.get_name().substr(0,3):
	                                           boost::lexical_cast<std::string>(msg.get_code());

	file << VERSION   << " "
	     << time      << " "
	     << acct      << " "
	     << nick      << " "
	     << type      << " "
	     << "\n";

	flush();
}
catch(const std::ios_base::failure &f)
{
	std::cerr << "Logging failure [" << get_path() << "]: " << f.what() << std::endl;
}
