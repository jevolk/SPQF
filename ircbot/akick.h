/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class AKick
{
	Mask mask;
	std::string oper;
	std::string reason_pub;
	std::string reason_priv;
	time_t expires;                           // Absolute time of expiration (not 'seconds from now')
	time_t modified;                          // Absolute time of modified (not 'seconds ago')

  public:
	using Type = Mask::Type;

	auto &get_mask() const                    { return mask;               }
	auto &get_oper() const                    { return oper;               }
	auto &get_reason_pub() const              { return reason_pub;         }
	auto &get_reason_priv() const             { return reason_priv;        }
	auto &get_expires() const                 { return expires;            }
	auto &get_modified() const                { return modified;           }

	bool operator<(const AKick &o) const      { return mask < o.mask;      }
	bool operator==(const AKick &o) const     { return mask == o.mask;     }

	AKick(const Mask &mask,
	      const std::string &oper,
	      const std::string &reason_pub,
	      const std::string &reason_priv,
	      const time_t &expires,
	      const time_t &modified):
	mask(mask),
	oper(oper),
	reason_pub(reason_pub),
	reason_priv(reason_priv),
	expires(expires),
	modified(modified)
	{

	}

	friend std::ostream &operator<<(std::ostream &s, const AKick &a)
	{
		s << std::setw(64) << std::left << a.get_mask();
		s << " by: " << std::setw(16) << std::left <<  a.get_oper();
		s << " reason (public): \"" << a.get_reason_pub() << "\"";
		s << " reason (private): \"" << a.get_reason_priv() << "\"";
		s << " expires: " << a.get_expires();
		s << " modified: " << a.get_modified();
		return s;
	}
};
