/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


using Tokens = std::vector<const std::string *>;
using Selection = std::forward_list<Tokens::const_iterator>;

Selection karma_tokens(const Tokens &tokens, const Chan &c, const std::string &oper = "++");
std::string detok(const Tokens &tokens);
Tokens subtok(const std::vector<std::string> &tokens);
Tokens subtok(const Tokens &tokens);


class ResPublica
{
	using Meth = Locutor::Method;

	static constexpr auto flush = Locutor::flush;

	Bot &bot;
	Opts &opts;
	Sess &sess;
	Users &users;
	Chans &chans;
	Events &events;
	Logs logs;
	Vdb vdb;
	Praetor praetor;
	Voting voting;

	// !vote abstract stack
	void handle_vote_info(const Msg &m, const User &u, Locutor &out, const Tokens &t, const Vote &vote);
	void handle_vote_list(const Msg &m, const User &u, Locutor &out, const Tokens &t, const id_t &id);
	void handle_help(const Msg &m, Locutor &out, const Tokens &t);

	// Privmsg stack
	void handle_vote_ballot(const Msg &m, User &u, const Tokens &t, const Vote::Ballot &b);
	void handle_vote_list(const Msg &m, User &u, const Tokens &t);
	void handle_vote_id(const Msg &m, User &u, const Tokens &t);
	void handle_vote(const Msg &m, User &u, const Tokens &t);
	void handle_version(const Msg &m, User &u, const Tokens &t);
	void handle_praetor(const Msg &m, User &u, const Tokens &t);
	void handle_regroup(const Msg &m, User &u, const Tokens &t);
	void handle_whoami(const Msg &m, User &u, const Tokens &t);
	void handle_config(const Msg &m, User &u, const Tokens &t);
	void handle_cmd(const Msg &m, User &u);

	// !vote channel stack
	void handle_vote_eligible(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_config(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_list(const Msg &m, Chan &c, User &u, const Tokens &t, const id_t &id);
	void handle_vote_list(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_ballot(const Msg &m, Chan &c, User &u, const Tokens &t, const Vote::Ballot &b);
	void handle_vote_cancel(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_id(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_version(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_cmd(const Msg &m, Chan &c, User &u);

	// Primary dispatch
	void handle_mlock(const Msg &m, Chan &c);
	void handle_knock(const Msg &m, Chan &c);
	void handle_on_chan(const Msg &m, Chan &c, User &u);
	void handle_not_op(const Msg &m, Chan &c);
	void handle_nick(const Msg &m, User &u);
	void handle_notice(const Msg &m, User &u);
	void handle_privmsg(const Msg &m, User &u);
	void handle_notice(const Msg &m, Chan &c, User &u);
	void handle_privmsg(const Msg &m, Chan &c, User &u);

  public:
	ResPublica(Bot &bot);
	~ResPublica() noexcept;
};
