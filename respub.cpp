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
                             Chan &chan)
{


}


void ResPublica::handle_mode(const Msg &msg,
                             Chan &chan,
                             User &user)
{


}


void ResPublica::handle_join(const Msg &msg,
                             Chan &chan,
                             User &user)
{


}


void ResPublica::handle_part(const Msg &msg,
                             Chan &chan,
                             User &user)
{


}


void ResPublica::handle_kick(const Msg &msg,
                             Chan &chan,
                             User &user)
{


}


void ResPublica::handle_cnotice(const Msg &msg,
                                Chan &chan,
                                User &user)
{


}


void ResPublica::handle_chanmsg(const Msg &msg,
                                Chan &chan,
                                User &user)
try
{
	const std::string &text = msg[CHANMSG::TEXT];

	// Discard empty without exception
	if(text.empty())
		return;

	// Dispatch based on first character
	switch(text.at(0))
	{
		case '!':      handle_chanmsg_cmd(msg,chan,user);                     break;
		default:                                                              break;
	}
}
catch(const Exception &e)
{
	chan << Meth::NOTICE << "Error with your command: " << e << flush;
}
catch(const std::out_of_range &e)
{
	chan << Meth::NOTICE << "You did not supply required arguments. Use the help command." << flush;
}


void ResPublica::handle_chanmsg_cmd(const Msg &msg,
                                    Chan &chan,
                                    User &user)
{
	using delim = boost::char_separator<char>;
	const delim sep(" ");

	const std::string &text = msg[CHANMSG::TEXT];
	const boost::tokenizer<delim> tokenize(text,sep);
	const std::vector<std::string> tokens(tokenize.begin(),tokenize.end());
	const Tokens subtoks = subtokenize(tokens);

	// Chop off cmd character and dispatch
	switch(hash(tokens.at(0).substr(1)))
	{
		case hash("vote"):     handle_vote(msg,chan,user,subtoks);            break;
		default:                                                              break;
	}
}


void ResPublica::handle_vote(const Msg &msg,
                             Chan &chan,
                             User &user,
                             const Tokens &toks)
{
	const std::string subcmd = toks.size()? *toks.at(0) : "help";
	const Tokens subtoks = subtokenize(toks);

	switch(hash(subcmd))
	{
		case hash("help"):     handle_vote_help(msg,chan,user,subtoks);          break;
		case hash("config"):   handle_vote_config(msg,chan,user,subtoks);        break;
		case hash("kick"):     handle_vote_kick(msg,chan,user,subtoks);          break;
		default:               handle_vote_mode(msg,chan,user,toks);             break;
	}
}


static const char *help_vote = \
"*** SENATVS POPVLVS QVE FREENODUS - The Senate and The People of Freenode ( #SPQF )\n"\
"*** Voting System:\n"\
"*** usage:    !vote   <command | issue>       : Initiate a vote for an issue or supply a command...\n"\
"*** issue:    <mode>  [target ...]            : Vote on setting modes for channel or target (+b implies kick)\n"\
"*** issue:    kick    <target>                : Vote to kick a target from the channel\n"\
"*** issue:    config  <variable = value>      : Vote on the voting configuration in this channel\n"\
"*** command:  config  [variable ...]          : Show whole configuration or one or more variable\n"\
;

static const char *help_vote_config = \
"** Vote on changing various configuration settings.\n"\
"** The syntax for initiating a vote is: <variable> = <new value>\n"\
"** The alternative syntax for printing information is: <variable> [variable ...]\n"\
"** Example: !vote config vote_minimum_quorum = 10  : requires at least 10 participants in any vote\n"\
;

static const char *help_vote_kick = \
"** Vote to kick user(s) from the channel.\n"\
"** The syntax for initiating a vote is: [target ...]\n"\
"** Example: !vote kick fred waldo             : Starts a single vote to kick both fred and waldo"\
;

static const char *help_vote_mode = \
"** Vote on changing modes for a channel with operator syntax.\n"\
"** The syntax for initiating a vote is: <mode string> [target ...]\n"\
"** Example: !vote +q $a:foobar     : Quiets based on the account foobar\n"\
"** Example: !vote +b <nickname>    : Kick-ban based on a nickname's $a: and/or *!*@host all in one\n"\
"** Example: !vote +r               : Set the channel for registered users only\n"\
;


void ResPublica::handle_vote_help(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	const std::string &what = toks.size()? *toks.at(0) : "";
	const Tokens subtoks = subtokenize(toks);

	Locutor &out = chan;

	switch(hash(what))
	{
		case hash("config"):   out << help_vote_config  << flush;               break;
		case hash("kick"):     out << help_vote_kick    << flush;               break;
		case hash("mode"):     out << help_vote_mode    << flush;               break;
		default:               out << help_vote         << flush;               break;
	}
}


void ResPublica::handle_vote_config(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks)
{
	if(toks.empty())
	{
		handle_vote_config_dump(msg,chan,user);
		return;
	}

	const std::string &what = *toks.at(0);
	const Tokens subtoks = subtokenize(toks);

	for(const auto &tok : toks)
		chan << "Config token [" << *tok << "]" << flush;
}



void ResPublica::handle_vote_config_dump(const Msg &msg,
                                         Chan &chan,
                                         User &user)
{



}


void ResPublica::handle_vote_kick(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	const std::string &target = *toks.at(0);
	chan << "Vote to kick: " << target << flush;
}


void ResPublica::handle_vote_mode(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	for(const auto &tok : toks)
		chan << "Mode token: [" << *tok << "]" << flush;
}


void ResPublica::handle_notice(const Msg &msg,
                               User &user)
{


}


void ResPublica::handle_privmsg(const Msg &msg,
                                User &user)
{


}


ResPublica::Tokens ResPublica::subtokenize(const Tokens &tokens)
{
	return {tokens.begin() + !tokens.empty(), tokens.end()};
}


ResPublica::Tokens ResPublica::subtokenize(const std::vector<std::string> &tokens)
{
	const bool off = !tokens.empty();
	Tokens ret(tokens.size() - off);
	std::transform(tokens.begin() + off,tokens.end(),ret.begin(),[]
	(const std::string &token)
	{
		return &token;
	});

	return ret;
}


ResPublica::Selection ResPublica::karma_tokens(const Tokens &tokens,
                                               const Chan &chan,
                                               const std::string &oper)
{
	Selection ret;
	for(auto it = tokens.begin(); it != tokens.end(); ++it)
	{
		const std::string &token = **it;
		if(token.find_last_of(oper) == std::string::npos)
			continue;

		const std::string nick = token.substr(0,token.size()-oper.size());
		if(chan.has_nick(nick))
			ret.push_front(it);
	}

	return ret;
}

