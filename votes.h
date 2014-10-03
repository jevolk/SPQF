/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


namespace vote
{
	class Config : public Vote
	{
		void passed();

	  public:
		template<class... Args> Config(Args&&... args);
	};

	class Kick : public Vote
	{
		void passed();

	  public:
		template<class... Args> Kick(Args&&... args);
	};
}


template<class... Args>
vote::Kick::Kick(Args&&... args):
Vote(std::forward<Args>(args)...)
{

}


inline
void vote::Kick::passed()
{
	Chan &chan = get_chan();
	const User &user = get_users().get(get_issue());
	chan.kick(user,"Voted off the island");
}


template<class... Args>
vote::Config::Config(Args&&... args):
Vote(std::forward<Args>(args)...)
{

}


inline
void vote::Config::passed()
{
	Chan &chan = get_chan();


}
