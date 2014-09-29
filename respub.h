/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */

class ResPublica : public Bot
{
	//static bool nick_has_flags(const std::string &nick);

	//size_t get_votes_against(const std::string &nick) const;
	//size_t inc_votes_against(const std::string &nick);

	//void handle_votekick(const std::string &target);

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
