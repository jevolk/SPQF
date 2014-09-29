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

/*
	void handle_endofnames(const Msg &msg) override;
	void handle_namreply(const Msg &msg) override;
	void handle_chan_notice(const Msg &msg) override;
	void handle_notice(const Msg &msg) override;
	void handle_privmsg(const Msg &msg) override;
	void handle_channel(const Msg &msg) override;
	void handle_kick(const Msg &msg) override;
	void handle_umode(const Msg &msg) override;
	void handle_mode(const Msg &msg) override;
	void handle_part(const Msg &msg) override;
	void handle_join(const Msg &msg) override;
*/

  public:
	template<class... Args> ResPublica(Args&&... args): Bot(std::forward<Args>(args)...) {}
};
