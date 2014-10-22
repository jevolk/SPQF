/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#ifndef LIBIRCBOT_INCLUDE
#define LIBIRCBOT_INCLUDE


#include <stdint.h>
#include <set>
#include <map>
#include <list>
#include <vector>
#include <forward_list>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

// boost
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

// ircclient
#include <libircclient.h>
#include <libirc_rfcnumeric.h>

// leveldb
#include <stldb/stldb.h>


// irc::bot
namespace irc {
namespace bot {

// bot.cpp based externs
extern std::locale locale;

namespace colors
{
#include "colors.h"
}
#include "util.h"
#include "callbacks.h"
#include "opts.h"
#include "mask.h"
#include "server.h"
#include "delta.h"
#include "mode.h"
#include "ban.h"
#include "flag.h"
#include "akick.h"
#include "msg.h"
#include "adoc.h"
#include "adb.h"
#include "acct.h"
#include "sess.h"
#include "locutor.h"
#include "service.h"
#include "user.h"
#include "log.h"
#include "chan.h"
#include "users.h"
#include "chans.h"
#include "nickserv.h"
#include "chanserv.h"
#include "logs.h"

/**
 * Primary libircbot object
 *
 * Usage:
 *	0. #include this file, and only this file, in your project.
 *	1. Override the given virtual handle_* functions in the protected section.
 *	2. Fill in an 'Opts' options structure (opts.h) and instance of this bot in your project.
 *	3. Operate the controls:
 *		conn() - initiate the connection to server
 *		run() - runs the event processing
 *		
 * This class is protected by a simple mutex:
 *	- Mutex is locked when handling events.
 *		+ The handlers you override operate under this lock.
 *	- If you access this class asynchronously outside of the handler stack you must lock.
 */
class Bot : public std::mutex
{
	Adb adb;
	Sess sess;
	Users users;
	Chans chans;
	NickServ ns;
	ChanServ cs;
	Logs logs;

  public:
	auto &get_adb() const                             { return adb;                         }
	auto &get_sess() const                            { return sess;                        }
	auto &get_opts() const                            { return sess.get_opts();             }
	auto &get_users() const                           { return users;                       }
	auto &get_chans() const                           { return chans;                       }
	auto &get_ns() const                              { return ns;                          }
	auto &get_cs() const                              { return cs;                          }
	auto &get_logs() const                            { return logs;                        }

	bool ready() const                                { return get_sess().is_conn();        }
	auto &get_nick() const                            { return get_sess().get_nick();       }
	bool my_nick(const std::string &nick) const       { return get_nick() == nick;          }

  protected:
	auto &get_adb()                                   { return adb;                         }
	auto &get_sess()                                  { return sess;                        }
	auto &get_users()                                 { return users;                       }
	auto &get_chans()                                 { return chans;                       }
	auto &get_ns()                                    { return ns;                          }
	auto &get_cs()                                    { return cs;                          }
	auto &get_logs()                                  { return logs;                        }

	// [RECV] Main interface for users of this library
	virtual void handle_privmsg(const Msg &m, User &u) {}
	virtual void handle_notice(const Msg &m, User &u) {}
	virtual void handle_action(const Msg &m, User &u) {}
	virtual void handle_chanmsg(const Msg &m, Chan &c, User &u) {}
	virtual void handle_caction(const Msg &m, Chan &c, User &u) {}
	virtual void handle_cnotice(const Msg &m, Chan &c, User &u) {}
	virtual void handle_kick(const Msg &m, Chan &c, User &u) {}
	virtual void handle_part(const Msg &m, Chan &c, User &u) {}
	virtual void handle_join(const Msg &m, Chan &c, User &u) {}
	virtual void handle_mode(const Msg &m, Chan &c, User &u) {}
	virtual void handle_mode(const Msg &m, Chan &c) {}

	// [RECV] Special override interface
	virtual void handle_caction_owner(const Msg &m, Chan &c, User &u);

  private:
	void log_handle(const Msg &m, const std::string &name = "") const;
	void handle_unhandled(const Msg &m);
	void handle_error(const Msg &m);

	void handle_notice_nickserv(const Msg &m);

	void handle_bannedfromchan(const Msg &m);
	void handle_chanoprivsneeded(const Msg &m);
	void handle_usernotinchannel(const Msg &m);
	void handle_useronchannel(const Msg &m);
	void handle_alreadyonchan(const Msg &m);
	void handle_channelmodeis(const Msg &m);
	void handle_topicwhotime(const Msg &m);
	void handle_creationtime(const Msg &m);
	void handle_endofnames(const Msg &m);
	void handle_namreply(const Msg &m);
	void handle_invitelist(const Msg &m);
	void handle_exceptlist(const Msg &m);
	void handle_quietlist(const Msg &m);
	void handle_banlist(const Msg &m);
	void handle_cnotice(const Msg &m);
	void handle_caction(const Msg &m);
	void handle_chanmsg(const Msg &m);
	void handle_knock(const Msg &m);
	void handle_notopic(const Msg &m);
	void handle_rpltopic(const Msg &m);
	void handle_topic(const Msg &m);
	void handle_mode(const Msg &m);
	void handle_kick(const Msg &m);
	void handle_part(const Msg &m);
	void handle_join(const Msg &m);
	void handle_ctcp_act(const Msg &m);
	void handle_ctcp_rep(const Msg &m);
	void handle_ctcp_req(const Msg &m);
	void handle_acceptnot(const Msg &msg);
	void handle_acceptexist(const Msg &msg);
	void handle_acceptfull(const Msg &msg);
	void handle_endofaccept(const Msg &msg);
	void handle_acceptlist(const Msg &msg);
	void handle_monlistfull(const Msg &m);
	void handle_endofmonlist(const Msg &m);
	void handle_monoffline(const Msg &m);
	void handle_mononline(const Msg &m);
	void handle_monlist(const Msg &m);
	void handle_unknownmode(const Msg &m);
	void handle_nicknameinuse(const Msg &m);
	void handle_erroneusnickname(const Msg &m);
	void handle_nosuchnick(const Msg &m);
	void handle_whowasuser(const Msg &m);
	void handle_endofwhois(const Msg &m);
	void handle_whoischannels(const Msg &m);
	void handle_whoisaccount(const Msg &m);
	void handle_whoissecure(const Msg &m);
	void handle_whoisserver(const Msg &m);
	void handle_whoisidle(const Msg &m);
	void handle_whoisuser(const Msg &m);
	void handle_whospecial(const Msg &m);
	void handle_whoreply(const Msg &m);
	void handle_umodeis(const Msg &m);
	void handle_notice(const Msg &m);
	void handle_privmsg(const Msg &m);
	void handle_action(const Msg &m);
	void handle_invite(const Msg &m);
	void handle_umode(const Msg &m);
	void handle_ison(const Msg &m);
	void handle_away(const Msg &m);
	void handle_nick(const Msg &m);
	void handle_quit(const Msg &m);
	void handle_authenticate(const Msg &m);
	void handle_account(const Msg &m);
	void handle_listend(const Msg &m);
	void handle_list(const Msg &m);
	void handle_cap(const Msg &m);
	void handle_myinfo(const Msg &m);
	void handle_created(const Msg &m);
	void handle_isupport(const Msg &m);
	void handle_yourhost(const Msg &m);
	void handle_welcome(const Msg &m);
	void handle_conn(const Msg &m);

	std::deque<Msg> dispatch_queue;
	std::mutex dispatch_mutex;
	std::condition_variable dispatch_cond;

	Msg dispatch_next();
	void dispatch(const Msg &m);
	void dispatch_worker();
	std::thread dispatch_thread;

  public:
	// Event/Handler input
	template<class... Msg> void operator()(Msg&&... msg);

	// Main controls
	void join(const std::string &chan)                { get_chans().join(chan);             }
	void quit()                                       { get_sess().quit();                  }
	void conn()                                       { get_sess().conn();                  }
	void run();                                             // Run worker loop

	Bot(void) = delete;
	Bot(const Opts &opts);
	Bot(Bot &&) = delete;
	Bot(const Bot &) = delete;
	Bot &operator=(Bot &&) = delete;
	Bot &operator=(const Bot &) = delete;
	virtual ~Bot(void) noexcept;

	friend std::ostream &operator<<(std::ostream &s, const Bot &bot);
};


template<class... Msg>
void Bot::operator()(Msg&&... args)
{
    const std::lock_guard<decltype(dispatch_mutex)> lock(dispatch_mutex);
    dispatch_queue.emplace_back(std::forward<Msg>(args)...);
    dispatch_cond.notify_one();
}


}       // namespace bot
}       // namespace irc

#endif  // LIBIRCBOT_INCLUDE
