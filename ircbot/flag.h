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

	auto &get_mask() const                  { return mask;               }
	auto &get_flags() const                 { return flags;              }
	auto &get_time() const                  { return time;               }
	auto &is_founder() const                { return founder;            }
	bool has(const char &c) const           { return flags.has(c);       }

	bool operator<(const Flag &o) const     { return mask < o.mask;      }
	bool operator==(const Flag &o) const    { return mask == o.mask;     }

	bool operator<(const Mask &o) const     { return mask < o;           }
	bool operator==(const Mask &o) const    { return mask == o;          }

	Flag(const Mask &mask     = Mask(),
	     const Mode &flags    = Mode(),
	     const time_t &time   = 0,
	     const bool &founder  = false):
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
