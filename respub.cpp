/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include <forward_list>
#include <condition_variable>
#include <thread>

#include "ircbot/bot.h"
#include "vote.h"
#include "votes.h"
#include "voting.h"
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
	const Adoc config = chan["config"];

	if(config.empty())
	{


	}
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
		case COMMAND_PREFIX:   handle_chanmsg_cmd(msg,chan,user);             break;
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
		case hash("yay"):      handle_vote_yay(msg,chan,user,subtoks);           break;
		case hash("nay"):      handle_vote_nay(msg,chan,user,subtoks);           break;
		case hash("poll"):     handle_vote_poll(msg,chan,user,subtoks);          break;
		case hash("help"):     handle_vote_help(msg,chan,user,subtoks);          break;
		case hash("config"):   handle_vote_config(msg,chan,user,subtoks);        break;
		case hash("kick"):     handle_vote_kick(msg,chan,user,subtoks);          break;
		default:               handle_vote_mode(msg,chan,user,toks);             break;
	}
}


void ResPublica::handle_vote_yay(const Msg &msg,
                                 Chan &chan,
                                 User &user,
                                 const Tokens &toks)
{
	auto &vote = voting.get(chan);

	switch(vote.vote_yay(user))
	{
		case Vote::ADDED:
			chan << user.get_nick() << ", thanks for casting your vote!" << flush;
			break;

		case Vote::CHANGED:
			chan << user.get_nick() << ", you have changed your vote to yay." << flush;
			break;

		default:
		case Vote::ALREADY:
			chan << user.get_nick() << ", you have already voted in this election." << flush;
			break;
	}
}


void ResPublica::handle_vote_nay(const Msg &msg,
                                 Chan &chan,
                                 User &user,
                                 const Tokens &toks)
{
	auto &vote = voting.get(chan);

	switch(vote.vote_nay(user))
	{
		case Vote::ADDED:
			chan << user.get_nick() << ", thanks for casting your vote!" << flush;
			break;

		case Vote::CHANGED:
			chan << user.get_nick() << ", you have changed your vote to nay." << flush;
			break;

		default:
		case Vote::ALREADY:
			chan << user.get_nick() << ", you have already voted in this election." << flush;
			break;
	}
}


void ResPublica::handle_vote_poll(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	const auto &vote = voting.get(chan);
	const auto tally = vote.tally();

	chan << "Current tally: ";
	chan << "YAY: " << tally.first << " ";
	chan << "NAY: " << tally.second << ". ";
	chan << "There are " << vote.remaining() << " seconds left. ";

	if(vote.total() < vote.minimum())
		chan << (vote.minimum() - vote.total()) << " more votes are required. ";
	else if(tally.first < vote.required())
		chan << (vote.required() < tally.first) << " more yays are required to pass. ";
	else
		chan << "As it stands, the motion will pass.";

	chan << flush;
}


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

	const std::string issue = detokenize(toks);
	voting.motion<vote::Config>(chan,get_users(),get_chans(),chan,issue);
}


void ResPublica::handle_vote_config_dump(const Msg &msg,
                                         Chan &chan,
                                         User &user)
{
	const Adoc cfg = chan.get("config");
	if(cfg.empty())
	{
		chan << "Channel has no configuration." << flush;
		return;
	}

	const Adoc vote_cfg = cfg.get_child("vote");
	if(vote_cfg.empty())
	{
		chan << "Channel has no voting configuration." << flush;
		return;
	}

	for(const auto &var : cfg)
		chan << var.first << " " << var.second << flush;
}


void ResPublica::handle_vote_kick(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	const std::string &target = *toks.at(0);
	voting.motion<vote::Kick>(chan,get_users(),get_chans(),chan,target);
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


std::string ResPublica::detokenize(const Tokens &tokens)
{
	std::stringstream str;
	for(const auto &token : tokens)
		str << *token;

	return str.str();
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

