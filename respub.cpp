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
#include <forward_list>
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
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <libircclient.h>
#include <libirc_rfcnumeric.h>

#include <leveldb/filter_policy.h>
#include <leveldb/cache.h>
#include <leveldb/db.h>

#include "util.h"
#include "ldb.h"
#include "mode.h"
#include "mask.h"
#include "ban.h"
#include "msg.h"
#include "adb.h"
#include "sess.h"
#include "locutor.h"
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
	// Just prints debugging to console
	std::cout << get_users() << std::endl << std::endl;
	std::cout << u << std::endl << std::endl;
	std::cout << c << std::endl << std::endl;

	using delim = boost::char_separator<char>;
	const delim sep(" ");

	const std::string &text = msg[CHANMSG::TEXT];
	const boost::tokenizer<delim> toks(text,sep);
	const Tokens tokens(toks.begin(),toks.end());

	const Selection good = karma_tokens(tokens,c,"++");
	const Selection bad = karma_tokens(tokens,c,"--");

	for(const auto &token : good)
		c << *token << " is good" << Chan::flush;

	for(const auto &token : bad)
		c << *token << " is bad" << Chan::flush;
}


void ResPublica::handle_notice(const Msg &msg,
                               User &u)
{


}


void ResPublica::handle_privmsg(const Msg &msg,
                                User &u)
{


}


Selection ResPublica::karma_tokens(const Tokens &tokens,
                                   const Chan &c,
                                   const std::string &oper)
{
	Selection ret;
	for(auto it = tokens.begin(); it != tokens.end(); ++it)
	{
		const std::string &token = *it;
		if(token.find_last_of(oper) == std::string::npos)
			continue;

		const std::string nick = token.substr(0,token.size()-oper.size());
		if(c.has_nick(nick))
			ret.push_front(it);
	}

	return ret;
}

