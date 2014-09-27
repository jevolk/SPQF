#include <stdint.h>
#include <vector>
#include <map>
#include <functional>
#include <iomanip>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <ostream>
#include <atomic>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <boost/tokenizer.hpp>

#include <libircclient.h>
#include <libirc_rfcnumeric.h>


/*******************************************************************************
 * Utilities
 */

constexpr
size_t hash(const char *const &str,
            const size_t i = 0)
{
    return !str[i]? 5381ULL : (hash(str,i+1) * 33ULL) ^ str[i];
}


struct scope
{
	typedef std::function<void ()> Func;
	const Func func;

	scope(const Func &func): func(func) {}
	~scope() { func(); }
};


class Exception : public std::runtime_error
{
	int c;

  public:
	const int &code() const  { return c; }

	Exception(const int &c = 0, const std::string &what = ""):
	          std::runtime_error(what), c(c) {}

	friend std::ostream &operator<<(std::ostream &s, const Exception &e)
	{
		s << e.what();
		return s;
	}
};


template<int CODE_FOR_SUCCESS = 0,
         class Exception = Exception,
         class Function,
         class... Args>
auto irc_call(irc_session_t *const &sess,
              Function&& func,
              Args&&... args)
-> decltype(func(sess,args...))
{
	const auto ret = func(sess,std::forward<Args>(args)...);

	if(ret != CODE_FOR_SUCCESS)
	{
		const int errc = irc_errno(sess);
		const char *const str = irc_strerror(errc);
		std::stringstream s;
		s << "libircclient: (" << errc << "): " << str;
		throw Exception(errc,s.str());
	}

	return ret;
}



/*******************************************************************************
 * Primary object
 */

class Bot
{
	// State
	std::string chan;
	std::string nick;
	std::string host;
	uint16_t port;

	irc_callbacks_t *cbs;
	irc_session_t *sess;

	// [SEND] libircclient call wrapper
	template<class F, class... A> auto irc_call(F&& f, A&&... a) -> decltype(::irc_call(sess,f,a...));
	template<class... VA_LIST> void quote(const char *const &fmt, VA_LIST&&... ap) __attribute__((format(printf,2,3)));

  public:
	// Observers
	const std::string &my_chan() const    { return chan;  }
	const std::string &my_nick() const    { return nick;  }
	const std::string &my_host() const    { return host;  }
	const uint16_t &my_port() const       { return port;  }
	bool is_conn() const;

	// Controls
	void disconn();
	void quit();
	void conn();

  protected:
	// Controls
	void me(const std::string &msg, const std::string &targ = "");
	void msg(const std::string &targ, const std::string &msg);
	void notice(const std::string &targ, const std::string &msg);
	void kick(const std::string &targ, const std::string &reason = "");
	void mode(const std::string &mode);
	void umode(const std::string &mode);
	void whois(const std::string &nick);
	void names();
	void part();
	void join();

	void msg(const std::string &msg);           // Message to channel
	void notice(const std::string &msg);        // Notice to channel
	void reply_chan(const std::string &msg);    // Reply to origin in channel
	void reply_priv(const std::string &msg);    // Private message to origin

	// Handler args (convenience)
	using Params = std::vector<std::string>;
	Params params;
	std::string origin;
	std::string origin_nick() const;
	std::string origin_host() const;

	void log_handle(const std::string &name, const uint32_t &code = 0, const std::string &remarks = "") const;
	void set_params(const char *const &origin, const char **const &params, const size_t &count);
	template<class Closure> void for_param(Closure&& closure);

	// [RECV] Handlers
	virtual void handle_unhandled(const uint32_t &code, const std::string &name);
	virtual void handle_endofnames();
	virtual void handle_namreply();
	virtual void handle_ctcp_act();
	virtual void handle_ctcp_rep();
	virtual void handle_ctcp_req();
	virtual void handle_invite();
	virtual void handle_chan_notice();
	virtual void handle_notice();
	virtual void handle_privmsg();
	virtual void handle_channel();
	virtual void handle_kick();
	virtual void handle_topic();
	virtual void handle_umode();
	virtual void handle_mode();
	virtual void handle_part();
	virtual void handle_join();
	virtual void handle_quit();
	virtual void handle_nick();
	virtual void handle_conn();

  public:
	// Event/Handler input
	void operator()(const uint32_t &event, const char *const  &origin, const char **const &params, const size_t &count);
	void operator()(const char *const &event, const char *const &origin, const char **const &params, const size_t &count);

	// Run worker loop
	void run();

	Bot(void) = delete;
	Bot(irc_callbacks_t &cbs, const std::string &chan, const std::string &nick, const std::string &host, const uint16_t &port);
	Bot(Bot &&) = delete;
	Bot(const Bot &) = delete;
	Bot &operator=(Bot &&) = delete;
	Bot &operator=(const Bot &) = delete;
	~Bot(void) noexcept;
};


Bot::Bot(irc_callbacks_t &cbs,
         const std::string &chan,
         const std::string &nick,
         const std::string &host,
         const uint16_t &port)
try:
chan(chan),
nick(nick),
host(host),
port(port),
cbs(&cbs),
sess(irc_create_session(&cbs))
{
	irc_set_ctx(sess,this);

}
catch(const Exception &e)
{
	std::cerr << "Bot::Bot(): " << e << std::endl;
}


Bot::~Bot(void)
noexcept try
{
	const scope free([&]
	{
		if(sess)
			irc_destroy_session(sess);
	});

	disconn();
}
catch(const Exception &e)
{
	std::cerr << "Bot::~Bot(): " << e << std::endl;
	return;
}


void Bot::operator()(const uint32_t &event,
                     const char *const &origin,
                     const char **const &params,
                     const size_t &count)
{
	set_params(origin,params,count);

	switch(event)
	{
		case LIBIRC_RFC_RPL_NAMREPLY:      handle_namreply();            break;
		case LIBIRC_RFC_RPL_ENDOFNAMES:    handle_endofnames();          break;
		default:                           handle_unhandled(event,"");   break;
	}

}


void Bot::operator()(const char *const &event,
                     const char *const &origin,
                     const char **const &params,
                     const size_t &count)
{
	set_params(origin,params,count);

	switch(hash(event))
	{
		case hash("CTCP_ACTION"):      handle_ctcp_act();           break;
		case hash("CTCP_REP"):         handle_ctcp_rep();           break;
		case hash("CTCP_REQ"):         handle_ctcp_req();           break;
		case hash("INVITE"):           handle_invite();             break;
		case hash("CHANNEL_NOTICE"):   handle_chan_notice();        break;
		case hash("NOTICE"):           handle_notice();             break;
		case hash("PRIVMSG"):          handle_privmsg();            break;
		case hash("CHANNEL"):          handle_channel();            break;
		case hash("KICK"):             handle_kick();               break;
		case hash("TOPIC"):            handle_topic();              break;
		case hash("UMODE"):            handle_umode();              break;
		case hash("MODE"):             handle_mode();               break;
		case hash("PART"):             handle_part();               break;
		case hash("JOIN"):             handle_join();               break;
		case hash("QUIT"):             handle_quit();               break;
		case hash("NICK"):             handle_nick();               break;
		case hash("CONNECT"):          handle_conn();               break;
		default:                       handle_unhandled(0,event);   break;
	}
}


void Bot::run()
{
	irc_call(irc_run);              // Loops forever here

	// see: handle_quit()
	std::cout << "Worker exit clean." << std::endl;
}


void Bot::handle_conn()
{
	log_handle("CONNECT");
	join();                         // Join the channel specified
}


void Bot::handle_nick()
{
	log_handle("NICK");
}


void Bot::handle_quit()
{
	log_handle("QUIT");

	//TODO: libbirc crashes if we don't exit (note: quit() was from a sighandler)
	exit(0);
}


void Bot::handle_join()
{
	log_handle("JOIN");
}


void Bot::handle_part()
{
	log_handle("PART");
}


void Bot::handle_mode()
{
	log_handle("MODE");
}


void Bot::handle_umode()
{
	log_handle("UMODE");
}


void Bot::handle_topic()
{
	log_handle("TOPIC");
}


void Bot::handle_kick()
{
	log_handle("KICK");
}


void Bot::handle_channel()
{
	log_handle("CHANNEL");
}


void Bot::handle_privmsg()
{
	log_handle("PRIVMSG");
}


void Bot::handle_notice()
{
	log_handle("NOTICE");
}


void Bot::handle_chan_notice()
{
	log_handle("CHANNEL NOTICE");
}


void Bot::handle_invite()
{
	log_handle("INVITE");
}


void Bot::handle_ctcp_req()
{
	log_handle("CTCP REQ");
}


void Bot::handle_ctcp_rep()
{
	log_handle("CTCP REP");
}


void Bot::handle_ctcp_act()
{
	log_handle("CTCP ACT");
}


void Bot::handle_namreply()
{
	log_handle("NAM REPLY");
}


void Bot::handle_endofnames()
{
	log_handle("END OF NAMES");
}


void Bot::handle_unhandled(const uint32_t &code,
                           const std::string &name)
{
	log_handle("UNHANDLED",code);
}


template<class Closure>
void Bot::for_param(Closure&& closure)
{
	for(const auto &param : params)
		closure(param);
}


void Bot::set_params(const char *const &origin,
                     const char **const &params,
                     const size_t &count)
{
	this->origin = origin;
	this->params = {params,params+count};
}


std::string Bot::origin_nick()
const
{
	char buf[32];
	irc_target_get_nick(origin.c_str(),buf,sizeof(buf));
	return buf;
}


std::string Bot::origin_host()
const
{
	char buf[128];
	irc_target_get_host(origin.c_str(),buf,sizeof(buf));
	return buf;
}


void Bot::log_handle(const std::string &name,
                     const uint32_t &code,
                     const std::string &remarks)
const
{
	std::cout << std::setw(15) << std::setfill(' ') << std::left << name;
	std::cout << "(" << std::setw(3) << code << ")";
	std::cout << " " << std::setw(27) << std::setfill(' ') << std::left << origin;
	std::cout << " (" << std::setw(2) << params.size() << "): ";

	for(const auto &param : params)
		std::cout << "[" << param << "]";

	std::cout << " " << remarks;
	std::cout << std::endl;
}


void Bot::conn()
{
	irc_call(irc_connect,host.c_str(),port,nullptr,nick.c_str(),nick.c_str(),nick.c_str());
}


void Bot::join()
{
	irc_call(irc_cmd_join,chan.c_str(),nullptr);
}


void Bot::part()
{
	irc_call(irc_cmd_part,chan.c_str());
}


void Bot::names()
{
	irc_call(irc_cmd_names,chan.c_str());
}


void Bot::whois(const std::string &nick)
{
	irc_call(irc_cmd_whois,nick.c_str());
}


void Bot::umode(const std::string &mode)
{
	irc_call(irc_cmd_user_mode,mode.c_str());
}


void Bot::mode(const std::string &mode)
{
	irc_call(irc_cmd_channel_mode,chan.c_str(),mode.c_str());
}


void Bot::kick(const std::string &targ,
               const std::string &reason)
{
	irc_call(irc_cmd_kick,targ.c_str(),chan.c_str(),reason.c_str());
}


void Bot::reply_chan(const std::string &text)
{
	std::stringstream s;
	s << origin_nick() << ": " << text;
	msg(chan,s.str());
}


void Bot::reply_priv(const std::string &text)
{
	msg(origin_nick(),text);
}


void Bot::msg(const std::string &text)
{
	msg(chan,text);
}


void Bot::notice(const std::string &text)
{
	notice(chan,text);
}


void Bot::notice(const std::string &targ,
                 const std::string &text)
{
	irc_call(irc_cmd_notice,targ.c_str(),text.c_str());
}


void Bot::msg(const std::string &targ,
              const std::string &text)
{
	irc_call(irc_cmd_msg,targ.c_str(),text.c_str());
}


void Bot::me(const std::string &text,
             const std::string &targ)
{
	const std::string &t = targ.empty()? nick : targ;
	irc_call(irc_cmd_me,t.c_str(),text.c_str());
}


void Bot::quit()
{
	irc_call(irc_cmd_quit,"K-Lined");
}


void Bot::disconn()
{
	irc_disconnect(sess);
}


bool Bot::is_conn() const
{
	return irc_is_connected(sess);
}


template<class... VA_LIST>
void Bot::quote(const char *const &fmt,
                VA_LIST&&... ap)
{
	irc_call(irc_send_raw,fmt,std::forward<VA_LIST>(ap)...);
}


template<class Func,
         class... Args>
auto Bot::irc_call(Func&& func,
                   Args&&... args)
-> decltype(::irc_call(sess,func,args...))
{
	return ::irc_call(sess,func,std::forward<Args>(args)...);
}



/*******************************************************************************
 * Democracy logic implementation
 */

struct User
{
	size_t votes_against = 0;

	User() {};
};


class ResPublica : public Bot
{
	std::map<std::string, User> users;

	static bool nick_has_flags(const std::string &nick);

	size_t get_votes_against(const std::string &nick) const;
	size_t inc_votes_against(const std::string &nick);

	void handle_votekick(const std::string &target);

	void handle_endofnames() override;
	void handle_namreply() override;
	void handle_chan_notice() override;
	void handle_notice() override;
	void handle_privmsg() override;
	void handle_channel() override;
	void handle_kick() override;
	void handle_umode() override;
	void handle_mode() override;
	void handle_part() override;
	void handle_join() override;

  public:
	template<class... Args> ResPublica(Args&&... args): Bot(std::forward<Args>(args)...) {}
};


void ResPublica::handle_join()
{
	log_handle("JOIN");
}


void ResPublica::handle_part()
{
	log_handle("PART");
}


void ResPublica::handle_mode()
{
	log_handle("MODE");
}


void ResPublica::handle_umode()
{
	log_handle("UMODE");
}


void ResPublica::handle_kick()
{
	log_handle("KICK");
}


void ResPublica::handle_channel()
{
	log_handle("CHANNEL");

	using delim = boost::char_separator<char>;
	const delim sep(" ");

	const std::string &channel = params.at(0);
	const std::string &text = params.at(1);
	const boost::tokenizer<delim> toks(text,sep);
	const std::vector<std::string> tokens(toks.begin(),toks.end());

	if(tokens.at(0) == "!votekick") try
	{
		const std::string &target = tokens.at(1);
		handle_votekick(target);
	}
	catch(const std::out_of_range &e)
	{
		reply_chan("Try using !votekick <nickname> to cast your vote.");
		return;
	}
}


void ResPublica::handle_votekick(const std::string &target)
{
	if(target == my_nick())
	{
		reply_chan("http://en.wikipedia.org/wiki/Leviathan_(book)");
		return;
	}

	const size_t votes = inc_votes_against(target);

	if(!votes)
	{
		reply_chan("I do not recognize that nickname.\n");
		return;
	}

	const size_t thresh = 2;

	std::stringstream s;
	s << "Thanks for your vote! ";
	s << "I count " << votes << " of " << thresh << " required to censure " << target << ".";
	reply_chan(s.str());

	if(votes >= thresh)
		kick(target,"The people have spoken.");
}


void ResPublica::handle_privmsg()
{
	log_handle("PRIVMSG");
}


void ResPublica::handle_notice()
{
	log_handle("NOTICE");
}


void ResPublica::handle_chan_notice()
{
	log_handle("CHANNEL NOTICE");
}


void ResPublica::handle_namreply()
{
	log_handle("NAME REPLY");
	const std::string &self = params.at(0);
	const std::string &equals_sign = params.at(1);
	const std::string &channel = params.at(2);

	std::for_each(params.begin()+3,params.end(),[&]
	(const std::string &names)
	{
		using delim = boost::char_separator<char>;
		const delim sep(" ");
		const boost::tokenizer<delim> tokens(names,sep);
		for(const auto &nick: tokens)
		{
			const std::string &n = nick_has_flags(nick)? nick.substr(1) : nick;
			users.emplace(std::piecewise_construct,
			              std::make_tuple(n),
			              std::make_tuple());
		}
	});
}


void ResPublica::handle_endofnames()
{
	log_handle("END OF NAMES");

	printf("Have %zu users\n",users.size());
	for(const auto &p : users)
		std::cout << p.first << " ";

	std::cout << std::endl;
}


size_t ResPublica::inc_votes_against(const std::string &nick)
try
{
	User &user = users.at(nick);
	return ++user.votes_against;
}
catch(const std::out_of_range &e)
{
	return 0;
}


size_t ResPublica::get_votes_against(const std::string &nick)
const try
{
	const User &user = users.at(nick);
	return user.votes_against;
}
catch(const std::out_of_range &e)
{
	return 0;
}


bool ResPublica::nick_has_flags(const std::string &nick)
try
{
	const char &c = nick.at(0);
	return c == '@' || c == '+';
}
catch(const std::out_of_range &e)
{
	return false;
}


/*******************************************************************************
 * Main / Globals
 */

template<class event_t>
void handler(irc_session_t *const &session,
                    event_t&& event,
                    const char *const &origin,
                    const char **params,
                    unsigned int &count)
try
{
	void *const ctx = irc_get_ctx(session);

	if(!ctx)
	{
		fprintf(stderr, "irc_get_ctx(session = %p) NULL CONTEXT!\n",ctx);
		return;
	}

	Bot &bot = *static_cast<Bot *>(ctx);
	bot(event,origin,params,count);
}
catch(const std::runtime_error &e)
{
	std::cerr
	<< "Unhandled exception: [" << e.what() << "] "
	<< "handler"
	<< "(" << session
	<< "," << event
	<< "," << origin
	<< "," << params
	<< "," << count
	<< ")";
}


static
void handler_numeric(irc_session_t *const session,
                     unsigned int event,
                     const char *const origin,
                     const char **params,
                     unsigned int count)
{
/*
	printf("DEBUG: (session: %p event: %u origin: %s params: %p count: %u)\n",
	       session,
	       event,
	       origin,
	       params,
	       count);
*/
	handler(session,event,origin,params,count);
}


static
void handler_named(irc_session_t *const session,
                   const char *const event,
                   const char *const origin,
                   const char **params,
                   unsigned int count)
{
/*
	printf("DEBUG: (session: %p event: %s origin: %s params: %p count: %u)\n",
	       session,
	       event,
	       origin,
	       params,
	       count);
*/
	handler(session,event,origin,params,count);
}


// Event handler globals for libircclient
static irc_callbacks_t cbs;

static
void init_cbs()
{
	cbs.event_numeric         = &handler_numeric;
	cbs.event_unknown         = &handler_named;
	cbs.event_connect         = &handler_named;
	cbs.event_nick            = &handler_named;
	cbs.event_quit            = &handler_named;
	cbs.event_join            = &handler_named;
	cbs.event_part            = &handler_named;
	cbs.event_mode            = &handler_named;
	cbs.event_umode           = &handler_named;
	cbs.event_topic           = &handler_named;
	cbs.event_kick            = &handler_named;
	cbs.event_channel         = &handler_named;
	cbs.event_privmsg         = &handler_named;
	cbs.event_notice          = &handler_named;
	cbs.event_channel_notice  = &handler_named;
	cbs.event_invite          = &handler_named;
	cbs.event_ctcp_req        = &handler_named;
	cbs.event_ctcp_rep        = &handler_named;
	cbs.event_ctcp_action     = &handler_named;
	//cbs.event_dcc_chat_req    = &handler_dcc;
	//cbs.event_dcc_send_req    = &handler_dcc;
}


// Pointer to instance for signals
static Bot *bot;

static
void handle_sig(const int sig)
{
	switch(sig)
	{
		case SIGINT:   printf("INTERRUPT...\n");  break;
		case SIGTERM:  printf("TERMINATE...\n");  break;
	}

	if(bot)
		bot->quit();
}


int main(int argc, char **argv)
try
{
	if(argc < 4)
	{
		printf("usage: ./%s <host> <port> <chan> [nick]\n",argv[0]);
		return -1;
	}

	const std::string host = argv[1];
	const uint16_t port = atoi(argv[2]);
	const std::string chan = argv[3];
	const std::string nick = argc > 4? argv[4] : "ResPublica";

	init_cbs();                                // Initialize static callback handlers
	ResPublica bot(cbs,chan,nick,host,port);   // Create instance of the bot
	::bot = &bot;                              // Set pointer for sighandlers
	signal(SIGINT,&handle_sig);                // Register handler for ctrl-c
	signal(SIGTERM,&handle_sig);               // Register handler for term
	bot.conn();                                // Connect to server (may throw)
	bot.run();                                 // Loops in foreground forever
}
catch(const Exception &e)
{
	std::cerr << "Exception: " << e << std::endl;
	return -1;
}
