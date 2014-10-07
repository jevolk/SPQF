/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


// libstd
#include <forward_list>
#include <condition_variable>
#include <thread>

// libircbot irc::bot::
#include "ircbot/bot.h"

// SPQF
using namespace irc::bot;
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
	chan << "Error with your command: " << e << flush;
}
catch(const std::out_of_range &e)
{
	chan << "You did not supply required arguments. Use the help command." << flush;
}


void ResPublica::handle_chanmsg_cmd(const Msg &msg,
                                    Chan &chan,
                                    User &user)
{
	using delim = boost::char_separator<char>;
	static const delim sep(" ");

	const std::string &text = msg[CHANMSG::TEXT];
	const boost::tokenizer<delim> tokenize(text,sep);
	const std::vector<std::string> tokens(tokenize.begin(),tokenize.end());
	const Tokens subtoks = subtokenize(tokens);

	// Chop off cmd character and dispatch
	switch(hash(tokens.at(0).substr(1)))
	{
		case hash("vote"):     handle_vote(msg,chan,user,subtoks);            break;
		case hash("config"):   handle_config(msg,chan,user,subtoks);          break;
		default:                                                              break;
	}
}


void ResPublica::handle_config(const Msg &msg,
                               Chan &chan,
                               User &user,
                               const Tokens &toks)
{
	const Adoc doc = chan.get();
	std::cout << doc << std::endl;
	chan << doc << flush;
}


void ResPublica::handle_vote(const Msg &msg,
                             Chan &chan,
                             User &user,
                             const Tokens &toks)
{
	const std::string subcmd = toks.size()? *toks.at(0) : "help";

	// Handle pattern for voting on config
	if(subcmd.find("config.") == 0)
	{
		handle_vote_config(msg,chan,user,toks);
		return;
	}

	// Handle pattern for voting on modes
	if(subcmd.at(0) == '+' || subcmd.at(0) == '-')
	{
		handle_vote_mode(msg,chan,user,toks);
		return;
	}

	// Handle various sub-keywords
	const Tokens subtoks = subtokenize(toks);
	switch(hash(subcmd))
	{
		case hash("yay"):
		case hash("yes"):
		case hash("yea"):
		case hash("Y"):
		case hash("y"):
		                       handle_vote_ballot(msg,chan,user,subtoks,Vote::YAY);      break;
		case hash("nay"):
		case hash("no"):
		case hash("N"):
		case hash("n"):
		                       handle_vote_ballot(msg,chan,user,subtoks,Vote::NAY);      break;
		case hash("poll"):     handle_vote_poll(msg,chan,user,subtoks);                  break;
		case hash("help"):     handle_vote_help(msg,chan,user,subtoks);                  break;
		case hash("cancel"):   handle_vote_cancel(msg,chan,user,subtoks);                break;
		case hash("config"):   handle_vote_config_dump(msg,chan,user,toks);              break;

		// Actual vote types
		case hash("kick"):     handle_vote_kick(msg,chan,user,subtoks);                  break;
		case hash("invite"):   handle_vote_invite(msg,chan,user,subtoks);                break;
		case hash("topic"):    handle_vote_topic(msg,chan,user,subtoks);                 break;
		default:               handle_vote_opine(msg,chan,user,toks);                    break;
	}
}


void ResPublica::handle_vote_ballot(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks,
                                    const Vote::Ballot &ballot)
{
	if(!toks.empty())
	{
		const auto &id = boost::lexical_cast<Vote::id_t>(*toks.at(0));
		auto &vote = voting.get(id);
		handle_vote_ballot(msg,chan,user,toks,ballot,vote);
	} else {
		auto &vote = voting.get(chan);
		handle_vote_ballot(msg,chan,user,toks,ballot,vote);
	}
}


void ResPublica::handle_vote_ballot(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks,
                                    const Vote::Ballot &ballot,
                                    Vote &vote)
try
{
	using namespace colors;

	switch(vote.vote(ballot,user))
	{
		case Vote::ADDED:
			chan << user << "Thanks for casting your vote on #" << BOLD << vote.get_id() << OFF << "!";
			break;

		case Vote::CHANGED:
			chan << user << "You have changed your vote on #" << BOLD << vote.get_id() << OFF << "!";
			break;
	}

	chan << flush;
}
catch(const Exception &e)
{
	using namespace colors;

	chan << user << "Your vote was not accepted for #" << BOLD << vote.get_id() << OFF
	             << ": " << e << flush;
	return;
}


void ResPublica::handle_vote_poll(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	using namespace colors;

	const Tokens subtoks = subtokenize(toks);

	if(toks.empty())
	{
		voting.for_each(chan,[&]
		(const Vote &vote)
		{
			const auto &id = vote.get_id();
			handle_vote_poll(msg,chan,user,subtoks,id);
		});

		return;
	}

	const auto &id = boost::lexical_cast<Vote::id_t>(toks.at(0));
	handle_vote_poll(msg,chan,user,subtoks,id);
}


void ResPublica::handle_vote_poll(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks,
                                  const id_t &id)
{
	using namespace colors;

	const auto &vote = voting.get(id);
	const auto tally = vote.tally();

	chan << "Current tally #" << BOLD << id << OFF << ": "
	     << BOLD << "YAY" << OFF << ": " << BOLD << FG::GREEN << tally.first << OFF << " "
	     << BOLD << "NAY" << OFF << ": " << BOLD << FG::RED << tally.second << OFF << " "
	     << "There are " << BOLD << vote.remaining() << BOLD << " seconds left. ";

	if(vote.total() < vote.minimum())
		chan << BOLD << (vote.minimum() - vote.total()) << OFF << " more votes are required. ";
	else if(tally.first < vote.required())
		chan << BOLD << (vote.required() < tally.first) << OFF << " more yays are required to pass. ";
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

	Locutor &out = user;

	switch(hash(what))
	{
		case hash("config"):   out << help_vote_config;   break;
		case hash("kick"):     out << help_vote_kick;     break;
		case hash("mode"):     out << help_vote_mode;     break;
		default:               out << help_vote;          break;
	}

	out << flush;
}


void ResPublica::handle_vote_cancel(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks)
try
{
	const Vote::id_t &id = boost::lexical_cast<Vote::id_t>(toks.at(0));
	voting.cancel(id,chan,user);
}
catch(const Exception &e)
{
	chan << user << e << flush;
}


void ResPublica::handle_vote_config_dump(const Msg &msg,
                                         Chan &chan,
                                         User &user,
                                         const Tokens &toks)
{
	const Adoc cfg = chan.get("config.vote");
	if(cfg.empty())
	{
		chan << "Channel has no voting configuration." << flush;
		return;
	}

	chan << cfg << flush;
}


void ResPublica::handle_vote_config(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks)
{
	const std::string issue = detokenize(toks);
	voting.motion<vote::Config>(chan,user,issue);
}


void ResPublica::handle_vote_mode(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	const std::string issue = detokenize(toks);
	voting.motion<vote::Mode>(chan,user,issue);
}


void ResPublica::handle_vote_kick(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	const std::string &target = *toks.at(0);
	voting.motion<vote::Kick>(chan,user,target);
}


void ResPublica::handle_vote_invite(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks)
{
	const std::string &target = *toks.at(0);
	voting.motion<vote::Invite>(chan,user,target);
}


void ResPublica::handle_vote_opine(const Msg &msg,
                                   Chan &chan,
                                   User &user,
                                   const Tokens &toks)
{
	const std::string issue = detokenize(toks);
	voting.motion<vote::Opine>(chan,user,issue);
}


void ResPublica::handle_vote_topic(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	const std::string issue = detokenize(toks);
	voting.motion<vote::Topic>(chan,user,issue);
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
	for(auto it = tokens.begin(); it != tokens.end(); ++it)
		str << **it << (it == tokens.end() - 1? "" : " ");

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

