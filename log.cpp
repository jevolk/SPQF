/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */

// libircbot irc::bot::
#include "ircbot/bot.h"
using namespace irc::bot;

// SPQF
#include "log.h"
using namespace irc::log;



///////////////////////////////////////////////////////////////////////////////
//
// log::Logs
//

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


bool Logs::exists(const std::string &name,
                  const Filter &filter)
const
{
	return !for_each(name,[&filter]
	(const ClosureArgs &a)
	{
		return !filter(a);
	});
}


bool Logs::atleast(const std::string &name,
                   const Filter &filter,
                   const size_t &count)
const
{
	size_t ret(0);
	return !count || !for_each(name,[&filter,&ret,&count]
	(const ClosureArgs &a)
	{
		ret += filter(a);
		return ret < count;
	});
}


size_t Logs::count(const std::string &name,
                   const Filter &filter)
const
{
	size_t ret(0);
	for_each(name,[&filter,&ret]
	(const ClosureArgs &a)
	{
		ret += filter(a);
		return true;
	});

	return ret;
}


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


bool Logs::for_each(const std::string &name,
                    const Closure &closure)
const try
{
	std::ifstream file;
	file.exceptions(std::ios_base::badbit);
	file.open(get_path(name),std::ios_base::in);

	while(1)
	{
		char buf[64] alignas(16);
		file.getline(buf,sizeof(buf));
		const size_t len = strlen(buf);
		std::replace(buf,buf+len,' ','\0');
		if(!len)
			return true;

		size_t i(0);
		const char *ptr(buf), *const end(buf + len);
		std::array<const char *, Log::_NUM_FIELDS> field;
		while(ptr < end)
		{
			field[i++] = ptr;
			ptr += strlen(ptr) + 1;
		}

		while(i < field.size())
			field[i++] = nullptr;

		if(__builtin_expect((!field[Log::VERS] || *field[Log::VERS] != '0'),0))
			throw Assertive("Log file corruption detected. I'm afraid I must abort...");

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


std::string Logs::get_path(const std::string &name)
const
{
	return opts["logdir"] + "/" + name;
}



///////////////////////////////////////////////////////////////////////////////
//
// log::Log
//

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


void Log::flush()
try
{
	file.flush();
}
catch(const std::ios_base::failure &f)
{
	throw Internal("Failed flushing log [") << get_path() << "]: " << f.what();
}


void Log::operator()(const Msg &msg,
                     const Chan &chan,
                     const User &user)
try
{
	static const uint VERSION(0);
	const time_t time(std::time(nullptr));
	const std::string &acct(user.is_logged_in()? user.get_acct() : "*");
	const std::string &nick(user.get_nick());
	const std::string &type(!msg.get_code()? msg.get_name().substr(0,3): lex_cast(msg.get_code()));
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



///////////////////////////////////////////////////////////////////////////////
//
// log:: misc
//

bool FilterAny::operator()(const ClosureArgs &args)
const
{
    return std::any_of(this->begin(),this->end(),[&args]
    (const auto &filter)
    {
        return filter(args);
    });
}


bool FilterAll::operator()(const ClosureArgs &args)
const
{
    return std::all_of(this->begin(),this->end(),[&args]
    (const auto &filter)
    {
        return filter(args);
    });
}


bool FilterNone::operator()(const ClosureArgs &args)
const
{
    return std::none_of(this->begin(),this->end(),[&args]
    (const auto &filter)
    {
        return filter(args);
    });
}
