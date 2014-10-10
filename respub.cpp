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


///////////////////////////////////////////////////////////////////////////////////
//
//   Primary dispatch (irc::bot::Bot overloads)
//

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


///////////////////////////////////////////////////////////////////////////////////
//
//   Channel message handlers
//

void ResPublica::handle_cmd(const Msg &msg,
                            Chan &chan,
                            User &user)
{
	using delim = boost::char_separator<char>;

	static const delim sep(" ");
	const boost::tokenizer<delim> tokenize(msg[CHANMSG::TEXT],sep);
	const std::vector<std::string> tok(tokenize.begin(),tokenize.end());

	// Chop off cmd character and dispatch
	switch(hash(tok.at(0).substr(1)))
	{
		case hash("vote"):     handle_vote(msg,chan,user,subtok(tok));    break;
		default:                                                          break;
	}
}


void ResPublica::handle_vote(const Msg &msg,
                             Chan &chan,
                             User &user,
                             const Tokens &toks)
{
	const std::string subcmd = !toks.empty()? *toks.at(0) : "help";

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
	switch(hash(subcmd))
	{
		// Ballot
		case hash("yes"):
		case hash("yea"):
		case hash("Y"):
		case hash("y"):        handle_vote_ballot(msg,chan,user,subtok(toks),Vote::YEA);   break;
		case hash("nay"):
		case hash("no"):
		case hash("N"):
		case hash("n"):        handle_vote_ballot(msg,chan,user,subtok(toks),Vote::NAY);   break;

		// Administrative
		case hash("count"):
		case hash("list"):     handle_vote_list(msg,chan,user,subtok(toks));               break;
		case hash("help"):     handle_vote_help(msg,user,subtok(toks));                    break;
		case hash("cancel"):   handle_vote_cancel(msg,chan,user,subtok(toks));             break;

		// Actual vote types
		case hash("ban"):      handle_vote_ban(msg,chan,user,subtok(toks));                break;
		case hash("unquiet"):  handle_vote_unquiet(msg,chan,user,subtok(toks));            break;
		case hash("quiet"):    handle_vote_quiet(msg,chan,user,subtok(toks));              break;
		case hash("kick"):     handle_vote_kick(msg,chan,user,subtok(toks));               break;
		case hash("invite"):   handle_vote_invite(msg,chan,user,subtok(toks));             break;
		case hash("topic"):    handle_vote_topic(msg,chan,user,subtok(toks));              break;
		default:               handle_vote_opine(msg,chan,user,toks);                      break;
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


void ResPublica::handle_vote_list(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	const Tokens subtoks = subtok(toks);

	if(toks.empty())
	{
		voting.for_each(chan,[&]
		(const Vote &vote)
		{
			const auto &id = vote.get_id();
			handle_vote_list(msg,chan,user,subtoks,id);
		});
	} else {
		const auto &id = boost::lexical_cast<Vote::id_t>(*toks.at(0));
		handle_vote_list(msg,chan,user,subtoks,id);
	}
}


void ResPublica::handle_vote_list(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks,
                                  const id_t &id)
{
	const Adoc &cfg = chan.get("config.vote.list");
	const bool ack_chan = cfg["ack_chan"] == "1";

	if(ack_chan)
		handle_vote_list(msg,user,chan,toks,id);
	else
		handle_vote_list(msg,user,user,toks,id);
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
	const std::string issue = detok(toks);
	if(issue.find("=") != std::string::npos)
	{
		voting.motion<vote::Config>(chan,user,issue);
		return;
	}

	const Adoc &cfg = chan.get();
	const std::string &key = *toks.at(0);
	const std::string &val = cfg[key];

	const bool ack_chan = cfg["config.config.ack_chan"] == "1";
	Locutor &out = ack_chan? static_cast<Locutor &>(chan):
	                         static_cast<Locutor &>(user);
	if(val.empty())
	{
		const Adoc &doc = cfg.get_child(key,Adoc());
		out << doc << flush;
	}
	else out << key << " = " << val << flush;
}


//////////////////////////////////////////////////////
//
//   Vote initiators
//

void ResPublica::handle_vote_mode(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	voting.motion<vote::Mode>(chan,user,detok(toks));
}


void ResPublica::handle_vote_kick(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	voting.motion<vote::Kick>(chan,user,detok(toks));
}


void ResPublica::handle_vote_ban(const Msg &msg,
                                 Chan &chan,
                                 User &user,
                                 const Tokens &toks)
{
	voting.motion<vote::Ban>(chan,user,detok(toks));
}


void ResPublica::handle_vote_quiet(const Msg &msg,
                                   Chan &chan,
                                   User &user,
                                   const Tokens &toks)
{
	voting.motion<vote::Quiet>(chan,user,detok(toks));
}


void ResPublica::handle_vote_unquiet(const Msg &msg,
                                     Chan &chan,
                                     User &user,
                                     const Tokens &toks)
{
	voting.motion<vote::UnQuiet>(chan,user,detok(toks));
}


void ResPublica::handle_vote_invite(const Msg &msg,
                                    Chan &chan,
                                    User &user,
                                    const Tokens &toks)
{
	voting.motion<vote::Invite>(chan,user,detok(toks));
}


void ResPublica::handle_vote_opine(const Msg &msg,
                                   Chan &chan,
                                   User &user,
                                   const Tokens &toks)
{
	voting.motion<vote::Opine>(chan,user,detok(toks));
}


void ResPublica::handle_vote_topic(const Msg &msg,
                                  Chan &chan,
                                  User &user,
                                  const Tokens &toks)
{
	voting.motion<vote::Topic>(chan,user,detok(toks));
}


///////////////////////////////////////////////////////////////////////////////////
//
//   Private message handlers
//

void ResPublica::handle_cmd(const Msg &msg,
                            User &user)
{
	using delim = boost::char_separator<char>;

	static const delim sep(" ");
	const boost::tokenizer<delim> tokenize(msg[PRIVMSG::TEXT],sep);
	const std::vector<std::string> tokens(tokenize.begin(),tokenize.end());
	const std::string &cmd = tokens.at(0).at(0) == COMMAND_PREFIX? tokens.at(0).substr(1):
	                                                               tokens.at(0);
	switch(hash(cmd))
	{
		case hash("vote"):     handle_vote(msg,user,subtok(tokens));     break;
		case hash("config"):   handle_config(msg,user,subtok(tokens));   break;
		default:                                                         break;
	}
}


void ResPublica::handle_config(const Msg &msg,
                               User &user,
                               const Tokens &toks)
try
{
	Chans &chans = get_chans();
	const Chan &chan = chans.get(*toks.at(0));
	const Adoc &cfg = chan.get();
	const std::string &key = toks.size() > 1? *toks.at(1) : "";
	const std::string &val = cfg[key];

	if(val.empty())
	{
		const Adoc &doc = cfg.get_child(key,Adoc());
		user << doc << flush;
	}
	else user << key << " = " << val << flush;
}
catch(const std::out_of_range &e)
{
	throw Exception("Need a channel name because this is PM.");
}


void ResPublica::handle_vote(const Msg &msg,
                             User &user,
                             const Tokens &toks)
{
	const std::string subcmd = !toks.empty()? *toks.at(0) : "help";
	switch(hash(subcmd))
	{
		// Ballot
		case hash("yea"):
		case hash("yay"):
		case hash("yes"):
		case hash("Y"):
		case hash("y"):        handle_vote_ballot(msg,user,subtok(toks),Vote::YEA);    break;
		case hash("nay"):
		case hash("no"):
		case hash("N"):
		case hash("n"):        handle_vote_ballot(msg,user,subtok(toks),Vote::NAY);    break;

		case hash("count"):
		case hash("list"):     handle_vote_list(msg,user,subtok(toks));                break;
		default:
		case hash("help"):     handle_vote_help(msg,user,subtok(toks));                break;
	}
}


void ResPublica::handle_vote_list(const Msg &msg,
                                  User &user,
                                  const Tokens &toks)
try
{
	Chans &chans = get_chans();
	const Chan &chan = chans.get(*toks.at(0));
	const Tokens subtoks = subtok(toks);

	if(subtoks.empty())
	{
		voting.for_each(chan,[&]
		(const Vote &vote)
		{
			const auto &id = vote.get_id();
			handle_vote_list(msg,user,user,subtoks,id);
		});
	} else {
		const auto &id = boost::lexical_cast<Vote::id_t>(*subtoks.at(0));
		handle_vote_list(msg,user,user,subtoks,id);
	}
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("You supplied a bad ID number.");
}
catch(const std::out_of_range &e)
{
	throw Exception("Need a channel name because this is PM.");
}


void ResPublica::handle_vote_ballot(const Msg &msg,
                                    User &user,
                                    const Tokens &toks,
                                    const Vote::Ballot &ballot)
try
{
	const Vote::id_t id = boost::lexical_cast<Vote::id_t>(*toks.at(0));
	auto &vote = voting.get(id);
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


///////////////////////////////////////////////////////////////////////////////////
//
//   Abstract handlers
//

void ResPublica::handle_vote_help(const Msg &msg,
                                  Locutor &out,
                                  const Tokens &toks)
{
	out << "http://pastebin.com/W6ZmqBG5";
	out << flush;
}


void ResPublica::handle_vote_list(const Msg &msg,
                                  User &user,
                                  Locutor &out,
                                  const Tokens &toks,
                                  const id_t &id)
{
	using namespace colors;

	const auto &vote = voting.get(id);
	const auto tally = vote.tally();

	out << vote << ": ";
	out << BOLD << "YEA" << OFF << ": " << BOLD << FG::GREEN << tally.first << OFF << " ";
	out << BOLD << "NAY" << OFF << ": " << BOLD << FG::RED << tally.second << OFF << " ";
	out << BOLD << "YOU" << OFF << ": ";

	if(!vote.voted(user))
		out << BOLD << FG::BLACK << "---" << OFF << " ";
	else if(vote.position(user) == Vote::YEA)
		out << BOLD << UNDER2 << FG::WHITE << BG::GREEN << "YEA" << OFF << " ";
	else if(vote.position(user) == Vote::NAY)
		out << BOLD << UNDER2 << FG::WHITE << BG::RED << "NAY" << OFF << " ";

	out << "There are " << BOLD << vote.remaining() << BOLD << " seconds left. ";

	if(vote.total() < vote.minimum())
		out << BOLD << (vote.minimum() - vote.total()) << OFF << " more votes are required. ";
	else if(tally.first < vote.required())
		out << BOLD << (vote.required() - tally.first) << OFF << " more yeas are required to pass. ";
	else
		out << "As it stands, the motion will pass.";

	out << flush;
}



///////////////////////////////////////////////////////////////////////////////////
//
//   Utils
//

Tokens subtok(const Tokens &tokens)
{
	return {tokens.begin() + !tokens.empty(), tokens.end()};
}


Tokens subtok(const std::vector<std::string> &tokens)
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



std::string detok(const Tokens &tokens)
{
	std::stringstream str;
	for(auto it = tokens.begin(); it != tokens.end(); ++it)
		str << **it << (it == tokens.end() - 1? "" : " ");

	return str.str();
}

Selection karma_tokens(const Tokens &tokens,
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
