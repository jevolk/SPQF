/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <unordered_map>
#include <functional>
#include <iomanip>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <ostream>
#include <atomic>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <libircclient.h>
#include <libirc_rfcnumeric.h>

#include "util.h"
#include "mode.h"
#include "mask.h"
#include "ban.h"
#include "msg.h"
#include "sess.h"
#include "user.h"
#include "chan.h"
#include "users.h"
#include "chans.h"
#include "bot.h"
#include "respub.h"


void ResPublica::handle_mode(const Msg &msg,
                             Chan &c)
{


}


void ResPublica::handle_mode(const Msg &msg,
                             Chan &c,
                             User &u)
{


}


void ResPublica::handle_join(const Msg &msg,
                             Chan &c,
                             User &u)
{


}


void ResPublica::handle_part(const Msg &msg,
                             Chan &c,
                             User &u)
{


}


void ResPublica::handle_kick(const Msg &msg,
                             Chan &c,
                             User &u)
{


}


void ResPublica::handle_cnotice(const Msg &msg,
                                Chan &c,
                                User &u)
{


}


void ResPublica::handle_chanmsg(const Msg &msg,
                                Chan &c,
                                User &u)
{
	using delim = boost::char_separator<char>;
	const delim sep(" ");

	const std::string &text = msg[CHANMSG::TEXT];
	const boost::tokenizer<delim> toks(text,sep);
	const std::vector<std::string> tokens(toks.begin(),toks.end());

	if(tokens.at(0) == "!ban") try
	{
		if(tokens.at(1) == "NICK")
			c.ban(u,Ban::Type::NICK);
		else if(tokens.at(1) == "HOST")
			c.ban(u,Ban::Type::HOST);
		else if(tokens.at(1) == "ACCT")
			c.ban(u,Ban::Type::ACCT);

		return;
	}
	catch(const std::out_of_range &e)
	{
		c.msg("specify");
		return;
	}

	if(tokens.at(0) == "!quiet") try
	{
		if(tokens.at(1) == "NICK")
			c.quiet(u,Ban::Type::NICK);
		else if(tokens.at(1) == "HOST")
			c.quiet(u,Ban::Type::HOST);
		else if(tokens.at(1) == "ACCT")
			c.quiet(u,Ban::Type::ACCT);

		return;
	}
	catch(const std::out_of_range &e)
	{
		c.msg("specify");
		return;
	}

	if(tokens.at(0) == "!unban") try
	{
		std::cout << c << std::endl;
		c.unban(u);
		std::cout << c << std::endl;
		return;
	}
	catch(const std::out_of_range &e)
	{
		c.msg("specify");
		return;
	}

	if(tokens.at(0) == "!unquiet") try
	{
		std::cout << c << std::endl;
		c.unquiet(u);
		std::cout << c << std::endl;
		return;
	}
	catch(const std::out_of_range &e)
	{
		c.msg("specify");
		return;
	}


	if(tokens.at(0) == "!accounttest")
	{
		c.for_each([&]
		(const User &user)
		{
			std::stringstream s;
			s << user.get_nick() << " is" << (!user.is_logged_in()? " not " : " ") << "logged in";
			if(user == u)
				s << " and made this query.";

			c.msg(s.str());
		});

		return;
	}

	if(tokens.at(0) == "!votekick") try
	{
		const std::string &target = tokens.at(1);
		c.msg("Thanks for voting");
	}
	catch(const std::out_of_range &e)
	{
		c.msg("Try using !votekick <nickname> to cast your vote.");
		return;
	}
}


void ResPublica::handle_notice(const Msg &msg,
                               User &u)
{


}


void ResPublica::handle_privmsg(const Msg &msg,
                                User &u)
{


}


/*
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
*/


/*
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
