/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */



using Tokens = std::vector<std::string>;
using Selection = std::forward_list<Tokens::const_iterator>;


class ResPublica : public Bot
{
	Selection karma_tokens(const Tokens &tokens, const Chan &c, const std::string &oper);


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
	template<class... Args> ResPublica(Args&&... args): Bot(std::forward<Args>(args)...) {}
};
