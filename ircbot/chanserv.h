/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class ChanServ : public Service
{
	Chans &chans;

	ChanServ &operator<<(const flush_t f) override;

	void captured(const Capture &capture);

  public:
	ChanServ(Adb &adb, Sess &sess, Chans &chans);
};


inline
ChanServ::ChanServ(Adb &adb,
                   Sess &sess,
                   Chans &chans):
Service(adb,sess,"ChanServ"),
chans(chans)
{


}


inline
void ChanServ::captured(const Capture &capture)
{


}


inline
ChanServ &ChanServ::operator<<(const flush_t f)
{
	Sess &sess = get_sess();
	auto &sendq = get_sendq();
	sess.chanserv(sendq.str());
	clear_sendq();
	return *this;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const ChanServ &cs)
{
	s << dynamic_cast<const Service &>(cs) << std::endl;
	return s;
}
