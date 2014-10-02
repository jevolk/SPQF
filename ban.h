/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Ban
{
	Mask mask;
	Mask oper;
	time_t time;

  public:
	using Type = Mask::Type;

	const Mask &get_mask() const           { return mask;               }
	const Mask &get_oper() const           { return oper;               }
	const time_t &get_time() const         { return time;               }

	bool operator<(const Ban &o) const     { return mask < o.mask;      }
	bool operator==(const Ban &o) const    { return mask == o.mask;     }

	Ban(const Mask &mask, const Mask &oper = "", const time_t &time = 0):
	    mask(mask), oper(oper), time(time) {}

	friend std::ostream &operator<<(std::ostream &s, const Ban &ban)
	{
		s << std::setw(64) << std::left << ban.get_mask();
		s << "by: " << std::setw(64) << std::left << ban.get_oper();
		s << "(" << ban.get_time() << ")";
		return s;
	}
};

using Quiet = Ban;
