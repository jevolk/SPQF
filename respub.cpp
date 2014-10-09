/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include <forward_list>

// libircbot irc::bot::
#include "ircbot/bot.h"

// SPQF
using namespace irc::bot;
#include "vote.h"
#include "votes.h"
#include "voting.h"
#include "respub.h"


void ResPublica::handle_privmsg(const Msg &msg,
                                User &user)
try
{
	const std::string &text = msg[PRIVMSG::TEXT];

	// Discard empty without exception
	if(text.empty())
		return;

	// Silently drop the background noise
	if(!user.is_logged_in())
		return;

	handle_cmd(msg,user);
}
catch(const Exception &e)
{
	user << "Failed: " << e << flush;
}
catch(const std::out_of_range &e)
{
	user << "You did not supply required arguments. Use the help command." << flush;
}


void ResPublica::handle_cmd(const Msg &msg,
                            User &user)
{
	using delim = boost::char_separator<char>;
	static const delim sep(" ");

	const std::string &text = msg[CHANMSG::TEXT];
	const boost::tokenizer<delim> tokenize(text,sep);
	const std::vector<std::string> tokens(tokenize.begin(),tokenize.end());
	const Tokens subtoks = subtokenize(tokens);

	const std::string &cmd = tokens.at(0).at(0) == COMMAND_PREFIX? tokens.at(0).substr(1):
	                                                               tokens.at(0);
	switch(hash(cmd))
	{
		case hash("vote"):     handle_vote(msg,user,subtoks);    break;
		case hash("config"):   handle_config(msg,user,subtoks);  break;
		default:                                                 break;
	}
}


void ResPublica::handle_config(const Msg &msg,
                               User &user,
                               const Tokens &toks)
{
	Chans &chans = get_chans();
	const Chan &chan = chans.get(*toks.at(0));
	const std::string &key = toks.size() > 1? *toks.at(1) : "";

	const Adoc &cfg = chan.get();
	const std::string &val = cfg[key];

	if(val.empty())
	{
		const Adoc &doc = cfg.get_child(key,Adoc());
		user << doc << flush;
	}
	else user << key << " = " << val << flush;
}


void ResPublica::handle_vote(const Msg &msg,
                             User &user,
                             const Tokens &toks)
{
	const Tokens subtoks = subtokenize(toks);
	const std::string subcmd = *toks.at(0);
	switch(hash(subcmd))
	{
		// Ballot
		case hash("yea"):
		case hash("yay"):
		case hash("yes"):
		case hash("Y"):
		case hash("y"):        handle_vote_ballot(msg,user,subtoks,Vote::YEA);      break;
		case hash("nay"):
		case hash("no"):
		case hash("N"):
		case hash("n"):        handle_vote_ballot(msg,user,subtoks,Vote::NAY);      break;
	}
}


void ResPublica::handle_vote_ballot(const Msg &msg,
                                    User &user,
                                    const Tokens &toks,
                                    const Vote::Ballot &ballot)
try
{
	auto &vote = voting.get(boost::lexical_cast<Vote::id_t>(*toks.at(0)));
	vote.vote(ballot,user);
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("You must supply the vote ID as a number.");
}
catch(const std::out_of_range &e)
{
	throw Exception("You must supply the vote ID given in the channel.");
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

	// Silently drop the background noise
	if(!user.is_logged_in())
		return;

	// Dispatch based on first character
	switch(text.at(0))
	{
		case COMMAND_PREFIX:   handle_cmd(msg,chan,user);             break;
		default:                                                      break;
	}
}
catch(const Exception &e)
{
	user << "Failed: " << e << flush;
}
catch(const std::out_of_range &e)
{
	user << "You did not supply required arguments. Use the help command." << flush;
}


void ResPublica::handle_cmd(const Msg &msg,
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
		case hash("vote"):     handle_vote(msg,chan,user,subtoks);    break;
		default:                                                      break;
	}
}


void ResPublica::handle_vote(const Msg &msg,
                             Chan &chan,
                             User &user,
                             const Tokens &toks)
{
	const std::string subcmd = toks.size()? *toks.at(0) : "help";

	// Handle pattern for voting on config
	if(subcmd.find("config") == 0)
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
		// Ballot
		case hash("yes"):
		case hash("yea"):
		case hash("Y"):
		case hash("y"):        handle_vote_ballot(msg,chan,user,subtoks,Vote::YEA);      break;
		case hash("nay"):
		case hash("no"):
		case hash("N"):
		case hash("n"):        handle_vote_ballot(msg,chan,user,subtoks,Vote::NAY);      break;

		// Administrative
		case hash("poll"):     handle_vote_poll(msg,chan,user,subtoks);                  break;
		case hash("help"):     handle_vote_help(msg,chan,user,subtoks);                  break;
		case hash("cancel"):   handle_vote_cancel(msg,chan,user,subtoks);                break;

		// Actual vote types
		case hash("ban"):      handle_vote_ban(msg,chan,user,subtoks);                   break;
		case hash("unquiet"):  handle_vote_unquiet(msg,chan,user,subtoks);               break;
		case hash("quiet"):    handle_vote_quiet(msg,chan,user,subtoks);                 break;
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
try
{
	static constexpr auto&& id_cast = boost::lexical_cast<Vote::id_t,std::string>;
	auto &vote = !toks.empty()? voting.get(id_cast(*toks.at(0))):
	                            voting.get(chan);
	vote.vote(ballot,user);
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("You must supply the vote ID as a number");
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
	} else {
		const auto &id = boost::lexical_cast<Vote::id_t>(*toks.at(0));
		handle_vote_poll(msg,chan,user,subtoks,id);
	}
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
	     << BOLD << "YEA" << OFF << ": " << BOLD << FG::GREEN << tally.first << OFF << " "
	     << BOLD << "NAY" << OFF << ": " << BOLD << FG::RED << tally.second << OFF << " "
	     << "There are " << BOLD << vote.remaining() << BOLD << " seconds left. ";

	if(vote.total() < vote.minimum())
		chan << BOLD << (vote.minimum() - vote.total()) << OFF << " more votes are required. ";
	else if(tally.first < vote.required())
		chan << BOLD << (vote.required() < tally.first) << OFF << " more yeas are required to pass. ";
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

		default:        out << "http://pastebin.com/W6ZmqBG5";     break;
	}

	out << flush;
}


void ResPublica::handle_vote_cancel(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks)
try
{
	static constexpr auto&& id_cast = boost::lexical_cast<Vote::id_t,std::string>;
	auto &vote = !toks.empty()? voting.get(id_cast(*toks.at(0))):
	                            voting.get(chan);
	voting.cancel(vote,chan,user);
}
catch(const Exception &e)
{
	chan << user << e << flush;
}


void ResPublica::handle_vote_config(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks)
{
	const std::string issue = detokenize(toks);
	if(issue.find("=") != std::string::npos)
	{
		voting.motion<vote::Config>(chan,user,issue);
		return;
	}

	const Adoc &cfg = chan.get();
	const bool ack_chan = cfg["config.config.ack_chan"] == "1";
	Locutor &out = ack_chan? static_cast<Locutor &>(chan) : static_cast<Locutor &>(user);

	const std::string &key = *toks.at(0);
	const std::string &val = cfg[key];

	if(val.empty())
	{
		const Adoc &doc = cfg.get_child(key,Adoc());
		out << doc << flush;
	}
	else out << key << " = " << val << flush;
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


void ResPublica::handle_vote_ban(const Msg &msg,
                                 Chan &chan,
                                 User &user,
                                 const Tokens &toks)
{
	const std::string &target = *toks.at(0);
	voting.motion<vote::Ban>(chan,user,target);
}


void ResPublica::handle_vote_quiet(const Msg &msg,
                                   Chan &chan,
                                   User &user,
                                   const Tokens &toks)
{
	const std::string &target = *toks.at(0);
	voting.motion<vote::Quiet>(chan,user,target);
}


void ResPublica::handle_vote_unquiet(const Msg &msg,
                                     Chan &chan,
                                     User &user,
                                     const Tokens &toks)
{
	const std::string &target = *toks.at(0);
	voting.motion<vote::UnQuiet>(chan,user,target);
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
