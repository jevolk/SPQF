/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class FloodGuard
{
	SendQ &sendq;
	const milliseconds saved;

  public:
	FloodGuard(Sess &sess, const milliseconds &inc);
	~FloodGuard();
};


inline
FloodGuard::FloodGuard(Sess &sess,
                       const milliseconds &inc):
sendq(sess.get_sendq()),
saved(sendq.get_throttle().get_inc())
{
	sendq.set_throttle(inc);
}


inline
FloodGuard::~FloodGuard()
{
	sendq.set_throttle(saved);
}
