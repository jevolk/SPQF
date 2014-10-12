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

	std::string get_nick() const         { return substr(0,find('!'));                           }
	std::string get_user() const         { return substr(find('!')+1,find('@') - find('!') - 1); }
	std::string get_host() const         { return substr(find('@')+1,npos);                      }
	bool has_ident() const               { return get_user().at(0) == '~';                       }
	bool has_wild_nick() const           { return get_nick() == "*";                             }
	bool has_wild_user() const           { return get_user() == "*";                             }
	bool has_wild_host() const           { return get_host() == "*";                             }
	bool has_all_wild() const
	{
		return has_wild_nick() &&
		       has_wild_user() &&
		       has_wild_host();
	}

	bool is_canonical() const
	{
		return find('!') != npos &&
		       find('@') != npos &&
		       rfind('!') == find('!') &&
		       rfind('@') == find('@') &&
		       find('!') < find('@') &&
		       !get_nick().empty() &&
		       !get_user().empty() &&
		       !get_host().empty();
	}

	bool has_negation() const            { return at(1) == '~';                                  }
	bool has_mask() const                { return find(':') != npos;                             }
	const char &get_exttype() const      { return has_negation()? at(2) : at(1);                 }
	std::string get_mask() const         { return has_mask()? substr(find(':')+1,npos) : "";     }
	bool has_wild_mask() const           { return get_mask().empty() || get_mask() == "*";       }
	bool is_extended() const
	{
		return size() > 1 &&
		       at(0) == '$';
	}

	Form get_form() const
	{
		return is_canonical()? CANONICAL:
		       is_extended()?  EXTENDED:
		                       INVALID;
	}

	template<class... A> Mask(A&&... a): std::string(std::forward<A>(a)...) {}
};
