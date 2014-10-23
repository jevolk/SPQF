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
	bool operator<(const Mask &o) const     { return mask < o;           }
	bool operator==(const Flag &o) const    { return mask == o.mask;     }
	bool operator==(const Mask &o) const    { return mask == o;          }

	template<class... Delta> Flag &operator+=(Delta&&... delta) &;
	template<class... Delta> Flag &operator-=(Delta&&... delta) &;

	bool delta(const std::string &str) &    { return flags.delta(str);   }
	void update(const time_t &time) &       { this->time = time;         }

	Flag(const Mask &mask     = Mask(),
	     const Mode &flags    = Mode(),
	     const time_t &time   = 0,
	     const bool &founder  = false):
	     mask(mask), flags(flags), time(time), founder(founder) {}

	Flag(const Delta &delta,
	     const time_t &time,
	     const bool &founder  = false);

	Flag(const Deltas &deltas,
	     const time_t &time,
	     const bool &founder  = false);

	friend std::ostream &operator<<(std::ostream &s, const Flag &f);
};


inline
Flag::Flag(const Delta &delta,
           const time_t &time,
           const bool &founder):
Flag(std::get<Delta::MASK>(delta),
     std::get<Delta::MODE>(delta),
     time,
     founder)
{
	if(!std::get<Delta::SIGN>(delta))
		throw Assertive("No reason to construct a negative flag at this time.");
}


inline
Flag::Flag(const Deltas &deltas,
           const time_t &time,
           const bool &founder):
mask(deltas.empty()? Mask() : std::get<Delta::MASK>(deltas.at(0))),
time(time),
founder(founder)
{
	if(!deltas.all_signs(true))
		throw Assertive("No reason to construct a negative flag at this time.");

	for(const Delta &d : deltas)
		flags.push_back(std::get<Delta::MODE>(d));
}


template<class... Delta>
Flag &Flag::operator+=(Delta&&... delta) &
{
	flags.operator+=(std::forward<Delta>(delta)...);
	return *this;
}


template<class... Delta>
Flag &Flag::operator-=(Delta&&... delta) &
{
	flags.operator-=(std::forward<Delta>(delta)...);
	return *this;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Flag &f)
{
	std::string mstr = f.get_flags();
	std::sort(mstr.begin(),mstr.end());
	s << "+" << std::setw(16) << std::left << mstr;
	s << " " << std::setw(64) << std::left << f.get_mask();
	s << " (" << f.get_time() << ")";

	if(f.is_founder())
		s << " (FOUNDER)";

	return s;
}
