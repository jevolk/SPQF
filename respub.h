/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


using Tokens = std::vector<const std::string *>;
using Selection = std::forward_list<Tokens::const_iterator>;

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
	Vdb vdb;
	Praetor praetor;
	Voting voting;

	void handle_unilateral_delta(const Msg &m, Chan &c, User &u, const Delta &d);
	void vote_stats_chan_user(Locutor &out, const std::string &chan, const std::string &user, const Tokens &t);
	void vote_stats_user(Locutor &out, const std::string &user, const Tokens &t);
	void vote_stats_chan(Locutor &out, const std::string &chan, const Tokens &t);
	void opinion(const Chan &c, const User &u, Locutor &out, const Tokens &t);
	void vote_list_oneline(const Chan &c, const User &u, Locutor &out, const std::list<id_t> &result);
	void handle_vote_info(const Msg &m, const User &u, Locutor &out, const Tokens &t, const Vote &vote);
	void handle_vote_list(const Msg &m, const User &u, Locutor &out, const Tokens &t, const id_t &id);
	void handle_vote_list(const Msg &m, const Chan &c, const User &u, Locutor &out, const Tokens &t);
	void handle_help(const Msg &m, Locutor &out, const Tokens &t);

	// Privmsg stack
	void handle_vote_stats(const Msg &m, User &u, const Tokens &t);
	void handle_vote_ballot(const Msg &m, User &u, const Tokens &t, const Vote::Ballot &b);
	void handle_vote_list(const Msg &m, User &u, const Tokens &t);
	void handle_vote_id(const Msg &m, User &u, const Tokens &t);
	void handle_vote(const Msg &m, User &u, Tokens t);
	void handle_opinion(const Msg &m, User &u, const Tokens &t);
	void handle_debug(const Msg &m, User &u, const Tokens &t);
	void handle_version(const Msg &m, User &u, const Tokens &t);
	void handle_praetor(const Msg &m, User &u, const Tokens &t);
	void handle_regroup(const Msg &m, User &u, const Tokens &t);
	void handle_whoami(const Msg &m, User &u, const Tokens &t);
	void handle_config(const Msg &m, User &u, const Tokens &t);
	void handle_cmd(const Msg &m, User &u);

	// !vote channel stack
	void handle_vote_stats(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_eligible(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_config(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_list(const Msg &m, Chan &c, User &u, const Tokens &t, const id_t &id);
	void handle_vote_list(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_ballot(const Msg &m, Chan &c, User &u, const Tokens &t, const Vote::Ballot &b);
	void handle_vote_cancel(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_id(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote(const Msg &m, Chan &c, User &u, Tokens t);
	void handle_opinion(const Msg &m, Chan &c, User &u, const Tokens &t);
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
	void handle_cmode(const Msg &m, Chan &c);
	void handle_notice(const Msg &m, Chan &c, User &u);
	void handle_privmsg(const Msg &m, Chan &c, User &u);

  public:
	ResPublica(Bot &bot);
	~ResPublica() noexcept;
};
