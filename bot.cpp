/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include <stdint.h>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
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

#include "util.h"
#include "mode.h"
#include "msg.h"
#include "sess.h"
#include "user.h"
#include "chan.h"
#include "users.h"
#include "chans.h"
#include "bot.h"


Bot::Bot(const Ident &ident,
         irc_callbacks_t &cbs)
try:
sess(ident,cbs),
chans(sess),
users(sess)
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
	sess.disconn();
}
catch(const Exception &e)
{
	std::cerr << "Bot::~Bot(): " << e << std::endl;
	return;
}


void Bot::operator()(const char *const &event,
                     const char *const &origin,
                     const char **const &params,
                     const size_t &count)
{
	const Msg msg(0,origin,params,count);

	switch(hash(event))
	{
		case hash("CTCP_ACTION"):      handle_ctcp_act(msg);            break;
		case hash("CTCP_REP"):         handle_ctcp_rep(msg);            break;
		case hash("CTCP_REQ"):         handle_ctcp_req(msg);            break;
		case hash("INVITE"):           handle_invite(msg);              break;
		case hash("CHANNEL_NOTICE"):   handle_cnotice(msg);             break;
		case hash("NOTICE"):           handle_notice(msg);              break;
		case hash("PRIVMSG"):          handle_privmsg(msg);             break;
		case hash("CHANNEL"):          handle_chanmsg(msg);             break;
		case hash("KICK"):             handle_kick(msg);                break;
		case hash("TOPIC"):            handle_topic(msg);               break;
		case hash("UMODE"):            handle_umode(msg);               break;
		case hash("MODE"):             handle_mode(msg);                break;
		case hash("PART"):             handle_part(msg);                break;
		case hash("JOIN"):             handle_join(msg);                break;
		case hash("QUIT"):             handle_quit(msg);                break;
		case hash("NICK"):             handle_nick(msg);                break;
		case hash("CONNECT"):          handle_conn(msg);                break;
		default:                       handle_unhandled(msg,event);     break;
	}
}


void Bot::operator()(const uint32_t &event,
                     const char *const &origin,
                     const char **const &params,
                     const size_t &count)
{
	const Msg msg(event,origin,params,count);

	switch(event)
	{
		case LIBIRC_RFC_RPL_NAMREPLY:             handle_namreply(msg);                break;
		case LIBIRC_RFC_RPL_ENDOFNAMES:           handle_endofnames(msg);              break;
		case LIBIRC_RFC_RPL_UMODEIS:              handle_umodeis(msg);                 break;
		case LIBIRC_RFC_RPL_CHANNELMODEIS:        handle_channelmodeis(msg);           break;

		case LIBIRC_RFC_ERR_ERRONEUSNICKNAME:     handle_erroneusnickname(msg);        break;
		case LIBIRC_RFC_ERR_BANNEDFROMCHAN:       handle_bannedfromchan(msg);          break;
		default:                                  handle_unhandled(msg);               break;
	}

}


void Bot::run()
{
	sess.call(irc_run);              // Loops forever here

	// see: handle_quit()
	std::cout << "Worker exit clean." << std::endl;
}


void Bot::handle_conn(const Msg &msg)
{
	log_handle(msg,"CONNECT");

	const Ident &id = get_sess().get_ident();
	for(const auto &chan : id.autojoin)
		join(chan);
}


void Bot::handle_nick(const Msg &msg)
{
	log_handle(msg,"NICK");

	const std::string old_nick = msg.get_nick();
	const std::string &new_nick = msg[NICK::NICKNAME];

	if(my_nick(old_nick))
	{
		Sess &sess = get_sess();
		sess.set_nick(new_nick);
	}

	Users &users = get_users();
	User &user = users.get(old_nick);

	Chans &chans = get_chans();
	chans.for_each([&user,&old_nick]
	(Chan &c)
	{
		c.rename(user,old_nick);
	});
}


void Bot::handle_quit(const Msg &msg)
{
	log_handle(msg,"QUIT");

	const std::string nick = msg.get_nick();
	const std::string &reason = msg[QUIT::REASON];

	if(my_nick(nick))
	{
		//TODO: libbirc crashes if we don't exit (note: quit() was from a sighandler)
		exit(0);
		return;
	}

	Users &users = get_users();
	User &user = users.get(nick);

	Chans &chans = get_chans();
	chans.for_each([&user]
	(Chan &chan)
	{
		chan.del(user);
	});
}


void Bot::handle_join(const Msg &msg)
{
	log_handle(msg,"JOIN");

	const std::string nick = msg.get_nick();
	const std::string &chan = msg[JOIN::CHANNAME];

	Users &users = get_users();
	User &u = users.add(nick);

	Chans &chans = get_chans();
	Chan &c = chans.add(chan);
	c.add(u);

	if(my_nick(nick))
	{
		c.set_joined(true);
		c.mode();
	}

	handle_join(msg,c,u);
}


void Bot::handle_part(const Msg &msg)
{
	log_handle(msg,"PART");

	const std::string nick = msg.get_nick();
	const std::string &chan = msg[PART::CHANNAME];
	const std::string &reason = msg[PART::REASON];

	Users &users = get_users();
	Chans &chans = get_chans();

	User &u = users.get(nick);
	Chan &c = chans.get(chan);
	c.del(u);

	if(my_nick(nick))
	{
		chans.del(chan);
		return;
	}

	const scope free([&]
	{
		if(u.num_chans() == 0)
			users.del(u);
	});

	handle_part(msg,c,u);
}


void Bot::handle_mode(const Msg &msg)
{
	log_handle(msg,"MODE");

	const std::string nick = msg.get_nick();

	// UMODE coming as MODE from the lib
	if(msg.num_params() == 1)
	{
		handle_umode(msg);
		return;
	}

	const std::string &chan = msg[MODE::CHANNAME];
	const std::string &mode = msg[MODE::DELTASTR];

	Chans &chans = get_chans();
	Chan &c = chans.get(chan);

	// Channel mode
	if(msg.num_params() < 3)
	{
		c.delta_mode(mode);
		handle_mode(msg,c);
		return;
	}

	// Channel user mode
	Users &users = get_users();
	const std::string sign(1,mode.at(0));
	for(size_t d = 1, m = 2; m < msg.num_params(); d++, m++) try
	{
		const std::string delta = sign + mode.at(d);
		const std::string &targ = msg[m];
		User &u = users.get(targ);
		c.delta_mode(u,delta);
		handle_mode(msg,c,u);
	}
	catch(const std::out_of_range &e)
	{
		std::cerr << "Mode update failed:"
		          << " chan: " << chan
		          << " target[" << m << "]: " << msg[m]
		          << " (modestr: " << mode << ")"
		          << std::endl;
	}
}


void Bot::handle_umode(const Msg &msg)
{
	log_handle(msg,"UMODE");

	const std::string nick = msg.get_nick();
	const std::string &mode = msg[UMODE::DELTASTR];

	if(!my_nick(nick))
		throw Exception("Server sent us umode for a different nickname");

	Sess &sess = get_sess();
	sess.delta_mode(mode);
}


void Bot::handle_umodeis(const Msg &msg)
{
	log_handle(msg,"UMODEIS");

	const std::string server = msg.get_host();
	const std::string &nick = msg[UMODEIS::NICKNAME];
	const std::string &mode = msg[UMODEIS::DELTASTR];

	if(!my_nick(nick))
		throw Exception("Server sent us umodeis for a different nickname");

	Sess &sess = get_sess();
	sess.delta_mode(mode);
}


void Bot::handle_channelmodeis(const Msg &msg)
{
	log_handle(msg,"CHANNELMODEIS");

	const std::string &self = msg[CHANNELMODEIS::NICKNAME];
	const std::string &chan = msg[CHANNELMODEIS::CHANNAME];
	const std::string &mode = msg[CHANNELMODEIS::DELTASTR];

	Chans &chans = get_chans();
	Chan &c = chans.get(chan);
	c.delta_mode(mode);
}


void Bot::handle_topic(const Msg &msg)
{
	log_handle(msg,"TOPIC");

}


void Bot::handle_kick(const Msg &msg)
{
	log_handle(msg,"KICK");

	const std::string kicker = msg.get_nick();
	const std::string &chan = msg[KICK::CHANNAME];
	const std::string &kickee = msg.num_params() > 1? msg[KICK::TARGET] : kicker;
	const std::string &reason = msg[KICK::REASON];

	Chans &chans = get_chans();

	if(my_nick(kickee))
	{
		chans.del(chan);
		chans.join(chan);
		return;
	}

	Users &users = get_users();

	User &u = users.get(kickee);
	Chan &c = chans.get(chan);
	c.del(u);

	const scope free([&]
	{
		if(u.num_chans() == 0)
			users.del(u);
	});

	handle_kick(msg,c,u);
}


void Bot::handle_chanmsg(const Msg &msg)
{
	log_handle(msg,"CHANMSG");

	const std::string &nick = msg.get_nick();
	const std::string &chan = msg[CHANMSG::CHANNAME];
	const std::string &txt = msg[CHANMSG::TEXT];

	Chans &chans = get_chans();
	Users &users = get_users();

	User &u = users.get(nick);
	Chan &c = chans.get(chan);

	handle_chanmsg(msg,c,u);
}


void Bot::handle_cnotice(const Msg &msg)
{
	log_handle(msg,"CHANNEL NOTICE");

	const std::string &nick = msg.get_nick();
	const std::string &chan = msg[CNOTICE::CHANNAME];
	const std::string &txt = msg[CNOTICE::TEXT];

	Chans &chans = get_chans();
	Users &users = get_users();

	User &u = users.get(nick);
	Chan &c = chans.get(chan);

	handle_cnotice(msg,c,u);
}


void Bot::handle_privmsg(const Msg &msg)
{
	log_handle(msg,"PRIVMSG");

	const std::string &nick = msg.get_nick();
	const std::string &self = msg[PRIVMSG::NICKNAME];
	const std::string &txt = msg[PRIVMSG::TEXT];

	Users &users = get_users();
	User &u = users.get(nick);
	handle_privmsg(msg,u);
}


void Bot::handle_notice(const Msg &msg)
{
	log_handle(msg,"NOTICE");

	if(msg.from_server())
		return;

	const std::string &nick = msg.get_nick();
	const std::string &targ = msg[NOTICE::NICKNAME];
	const std::string &txt = msg[NOTICE::TEXT];

	Users &users = get_users();
	User &u = users.get(targ);
	handle_notice(msg,u);
}


void Bot::handle_invite(const Msg &msg)
{
	log_handle(msg,"INVITE");
}


void Bot::handle_ctcp_req(const Msg &msg)
{
	log_handle(msg,"CTCP REQ");
}


void Bot::handle_ctcp_rep(const Msg &msg)
{
	log_handle(msg,"CTCP REP");
}


void Bot::handle_ctcp_act(const Msg &msg)
{
	log_handle(msg,"CTCP ACT");
}


void Bot::handle_namreply(const Msg &msg)
{
	log_handle(msg,"NAM REPLY");
	const std::string &self = msg[NAMREPLY::NICKNAME];
	const std::string &type = msg[NAMREPLY::TYPE];
	const std::string &chan = msg[NAMREPLY::CHANNAME];
	const std::string &names = msg[NAMREPLY::NAMELIST];

	if(!my_nick(self))
		throw Exception("Server replied to the wrong nickname");

	Users &users = get_users();
	Chans &chans = get_chans();
	Chan &c = chans.add(chan);

	using delim = boost::char_separator<char>;
	const delim sep(" ");
	const boost::tokenizer<delim> tokens(names,sep);
	for(const auto &nick: tokens)
	{
		const char f = Chan::nick_flag(nick);
		const char m = Chan::flag2mode(f);
		const std::string &rawnick = f? nick.substr(1) : nick;
		const std::string &modestr = m? std::string(1,m) : "";

		User &user = users.add(rawnick);
		c.add(user,modestr);
	}
}


void Bot::handle_endofnames(const Msg &msg)
{
	log_handle(msg,"END OF NAMES");
}


void Bot::handle_bannedfromchan(const Msg &msg)
{
	log_handle(msg,"BANNED FROM CHAN");
}


void Bot::handle_erroneusnickname(const Msg &msg)
{
	log_handle(msg,"ERRONEOUS NICKNAME");
	quit();
}


void Bot::handle_unhandled(const Msg &msg,
                           const std::string &name)
{
	log_handle(msg,name);
}


void Bot::handle_unhandled(const Msg &msg)
{
	log_handle(msg,msg.get_code());
}


void Bot::log_handle(const Msg &msg,
                     const uint32_t &code,
                     const std::string &remarks)
const
{
	std::cout << std::setw(15) << std::setfill(' ') << std::left << "";
	std::cout << msg;
	std::cout << " " << remarks;
	std::cout << std::endl;
}


void Bot::log_handle(const Msg &msg,
                     const std::string &name,
                     const std::string &remarks)
const
{
	std::cout << std::setw(15) << std::setfill(' ') << std::left << name;
	std::cout << msg;
	std::cout << " " << remarks;
	std::cout << std::endl;
}


std::ostream &operator<<(std::ostream &s,
                         const Bot &b)
{
	s << "Session: " << std::endl;
	s << b.sess << std::endl;
	s << b.chans << std::endl;
	s << b.users << std::endl;
	return s;
}
