/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Throttle
{
	time_point state;
	milliseconds inc;

  public:
	auto &get_inc() const                        { return inc;                  }
	void set_inc(const milliseconds &inc)        { this->inc = inc;             }
	void clear()                                 { inc = 0ms;                   }

	milliseconds calc_rel() const;
	time_point calc_abs() const;

	milliseconds next();
	time_point next_abs();

	Throttle(const milliseconds &inc = 0ms);
	Throttle(const uint64_t &inc): Throttle(milliseconds(inc)) {}
};


inline
Throttle::Throttle(const milliseconds &inc):
state(steady_clock::time_point::min()),
inc(inc)
{


}


inline
milliseconds Throttle::next()
{
	using namespace std::chrono;

	const auto now = steady_clock::now();

	if(state < now)
	{
		state = now + inc;
		return 0ms;
	}

	const auto ret = duration_cast<milliseconds>(state.time_since_epoch()) -
	                 duration_cast<milliseconds>(now.time_since_epoch());
	state += inc;
	return ret;
}


inline
time_point Throttle::next_abs()
{
	const auto now = steady_clock::now();
	const auto ret = state < now? now : state;
	state = ret + inc;
	return ret;
}


inline
milliseconds Throttle::calc_rel()
const
{
	using namespace std::chrono;

	const auto now = steady_clock::now();

	if(state < now)
		return 0ms;

	return duration_cast<milliseconds>(state.time_since_epoch()) -
	       duration_cast<milliseconds>(now.time_since_epoch());
}


inline
time_point Throttle::calc_abs()
const
{
	const auto now = steady_clock::now();
	return state < now? now : state + inc;
}
