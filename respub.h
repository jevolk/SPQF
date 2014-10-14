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


class ResPublica : public irc::bot::Bot
{
	using Meth = Locutor::Method;

	static constexpr auto &flush = Locutor::flush;

	Voting voting;

	// !vote abstract stack
	void handle_vote_list(const Msg &m, User &u, Locutor &out, const Tokens &t, const id_t &id);
	void handle_help(const Msg &m, Locutor &out, const Tokens &t);

	// Privmsg stack
	void handle_vote_ballot(const Msg &m, User &u, const Tokens &t, const Vote::Ballot &b);
	void handle_vote_list(const Msg &m, User &u, const Tokens &t);
	void handle_config(const Msg &m, User &u, const Tokens &toks);
	void handle_vote(const Msg &m, User &u, const Tokens &t);
	void handle_cmd(const Msg &m, User &u);

	// !vote channel stack
	void handle_vote_config(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_list(const Msg &m, Chan &c, User &u, const Tokens &t, const id_t &id);
	void handle_vote_list(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_ballot(const Msg &m, Chan &c, User &u, const Tokens &t, const Vote::Ballot &b);
	void handle_vote_cancel(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_cmd(const Msg &m, Chan &c, User &u);

	// Primary dispatch
	void handle_chanmsg(const Msg &m, Chan &c, User &u) override;
	void handle_privmsg(const Msg &m, User &u) override;

  public:
	template<class... Args> ResPublica(Args&&... args);
};


template<class... Args>
ResPublica::ResPublica(Args&&... args):
irc::bot::Bot(std::forward<Args>(args)...),
voting(*this,get_sess(),get_chans(),get_users(),get_logs())
{


}
