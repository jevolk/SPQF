/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Flag
{
	Mask mask;
	Mode flags;
	time_t time;
	bool founder;

  public:
	using Type = Mask::Type;

	const Mask &get_mask() const           { return mask;               }
	const Mode &get_flags() const          { return flags;              }
	const time_t &get_time() const         { return time;               }
	const bool &is_founder() const         { return founder;            }

	bool operator<(const Flag &o) const    { return mask < o.mask;      }
	bool operator==(const Flag &o) const   { return mask == o.mask;     }

	Flag(const Mask &mask, const Mode &flags, const time_t &time, const bool &founder = false):
	     mask(mask), flags(flags), time(time), founder(founder) {}

	friend std::ostream &operator<<(std::ostream &s, const Flag &f)
	{
		s << "+" << std::setw(16) << std::left << f.get_flags();
		s << " " << std::setw(64) << std::left << f.get_mask();
		s << " (" << f.get_time() << ")";

		if(f.is_founder())
			s << " (FOUNDER)";

		return s;
	}
};
