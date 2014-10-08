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
	const std::string &get_path() const          { return path;             }

	auto flush() -> decltype(file.flush())       { return file.flush();     }

	void operator()(const User &user, const std::string &msg);

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
                     const std::string &msg)
try
{
	static time_t time;
	std::time(&time);

	file << time << " "
	     << (user.is_logged_in()? user.get_acct() : "*") << " "
	     << user.get_nick() << " "
	     << msg << "\n";

	flush();
}
catch(const std::ios_base::failure &f)
{
	std::cerr << "Logging failure [" << get_path() << "]: " << f.what() << std::endl;
}
