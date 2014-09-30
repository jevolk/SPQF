/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Mask : public std::string
{
	enum Form                            { INVALID, CANONICAL, EXTENDED                          };
    enum Type                            { NICK, HOST, ACCT                                      };

	bool is_canonical() const            { return find('!') != npos && find('@') != npos;        }
	std::string get_nick() const         { return substr(0,find('!'));                           }
	std::string get_user() const         { return substr(find('!')+1,find('@') - find('!') - 1); }
	std::string get_host() const         { return substr(find('@')+1,npos);                      }
	bool has_wild_nick() const           { return get_nick() == "*";                             }
	bool has_wild_user() const           { return get_user() == "*";                             }
	bool has_wild_host() const           { return get_host() == "*";                             }
	bool has_ident() const               { return get_user().at(0) == '~';                       }

	bool is_extended() const             { return at(0) == '$';                                  }
	bool has_negation() const            { return at(1) == '~';                                  }
	bool has_mask() const                { return find(':') != npos;                             }
	const char &get_exttype() const      { return has_negation()? at(2) : at(1);                 }
	std::string get_mask() const         { return has_mask()? substr(find(':')+1,npos) : "";     }
	bool has_wild_mask() const           { return get_mask().empty() || get_mask() == "*";       }
	Form get_form() const
	{
		return is_canonical()? CANONICAL:
		       is_extended()?  EXTENDED:
		                       INVALID;
	}

	template<class... A> Mask(A&&... a): std::string(std::forward<A>(a)...) {}
};


template<class Mask>
class Masks
{
	std::set<Mask> s;

  public:
	const auto begin() const -> decltype(s.begin())  { return s.begin();                                }
	const auto end() const -> decltype(s.end())      { return s.end();                                  }

	const Mask &get(const Mask &mask)                { return s.at(mask);                               }
	bool has(const Mask &mask) const                 { return s.count(mask);                            }
	size_t num() const                               { return s.size();                                 }

	template<class... A> bool add(A&&... a)          { return s.emplace(std::forward<A>(a)...).second;  }
	bool del(const Mask &mask)                       { return s.erase(mask);                            }
};
