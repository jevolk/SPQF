/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class ResPublica : public irc::bot::Bot
{
	using Meth = Locutor::Method;

	using Tokens = std::vector<const std::string *>;
	using Selection = std::forward_list<Tokens::const_iterator>;

	static const char COMMAND_PREFIX = '!';
	static constexpr auto &flush = Locutor::flush;
	static Selection karma_tokens(const Tokens &tokens, const Chan &c, const std::string &oper);
	static Tokens subtokenize(const std::vector<std::string> &tokens);
	static Tokens subtokenize(const Tokens &tokens);
	static std::string detokenize(const Tokens &tokens);

	Voting voting;

	// Vote cmd stack
	void handle_vote_opine(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_topic(const Msg &m, Chan &c, User &u, const Tokens &toks);
	void handle_vote_invite(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_unquiet(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_quiet(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_ban(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_kick(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_mode(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_config(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_cancel(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_help(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_poll(const Msg &m, Chan &c, User &u, const Tokens &t, const id_t &id);
	void handle_vote_poll(const Msg &m, Chan &c, User &u, const Tokens &t);
	void handle_vote_ballot(const Msg &m, Chan &c, User &u, const Tokens &t, const Vote::Ballot &b);
	void handle_vote(const Msg &m, Chan &c, User &u, const Tokens &t);

	void handle_config(const Msg &m, Chan &c, User &u, const Tokens &toks);

	// Cmd prefix dispatch; default: '!'
	void handle_chanmsg_cmd(const Msg &m, Chan &c, User &u);

	// Primary dispatch
	void handle_privmsg(const Msg &m, User &u) override;
	void handle_notice(const Msg &m, User &u) override;
	void handle_chanmsg(const Msg &m, Chan &c, User &u) override;
	void handle_cnotice(const Msg &m, Chan &c, User &u) override;
	void handle_kick(const Msg &m, Chan &c, User &u) override;
	void handle_part(const Msg &m, Chan &c, User &u) override;
	void handle_join(const Msg &m, Chan &c, User &u) override;
	void handle_mode(const Msg &m, Chan &c, User &u) override;
	void handle_mode(const Msg &m, Chan &c) override;

  public:
	template<class... Args> ResPublica(Args&&... args);
};


template<class... Args>
ResPublica::ResPublica(Args&&... args):
irc::bot::Bot(std::forward<Args>(args)...),
voting(*this,get_sess(),get_chans(),get_users())
{


}
