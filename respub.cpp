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
#include "respub.h"


/*

void ResPublica::handle_join(const Msg &msg)
{
	log_handle(msg,"JOIN");
}


void ResPublica::handle_part(const Msg &msg)
{
	log_handle(msg,"PART");
}


void ResPublica::handle_mode(const Msg &msg)
{
	log_handle(msg,"MODE");
}


void ResPublica::handle_umode(const Msg &msg)
{
	log_handle(msg,"UMODE");
}


void ResPublica::handle_kick(const Msg &msg)
{
	log_handle(msg,"KICK");
}


void ResPublica::handle_channel(const Msg &msg)
{
	log_handle(msg,"CHANNEL");

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

void ResPublica::handle_privmsg(const Msg &msg)
{
	log_handle(msg,"PRIVMSG");
}


void ResPublica::handle_notice(const Msg &msg)
{
	log_handle(msg,"NOTICE");
}


void ResPublica::handle_chan_notice(const Msg &msg)
{
	log_handle(msg,"CHANNEL NOTICE");
}


void ResPublica::handle_namreply(const Msg &msg)
{
	log_handle(msg,"NAME REPLY");

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


void ResPublica::handle_endofnames(const Msg &msg)
{
	log_handle(msg,"END OF NAMES");

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
*/
