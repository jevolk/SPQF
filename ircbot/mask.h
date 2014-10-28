/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Mask : std::string
{
	static const size_t NICKNAME_MAX_LEN  = 16;
	static const size_t USERNAME_MAX_LEN  = 16;
	static const size_t HOSTNAME_MAX_LEN  = 127;

	enum Form                             { INVALID, CANONICAL, EXTENDED                          };
	enum Type                             { NICK, HOST, ACCT                                      };

	auto num(const char &c) const         { return std::count(begin(),end(),c);                   }
	bool has(const char &c) const         { return find(c) != std::string::npos;                  }

	// Canonical mask related
	std::string get_nick() const          { return substr(0,find('!'));                           }
	std::string get_user() const          { return substr(find('!')+1,find('@') - find('!') - 1); }
	std::string get_host() const          { return substr(find('@')+1,npos);                      }

	bool has_ident() const                { return !has('~');                                     }
	bool has_wild_nick() const            { return get_nick() == "*";                             }
	bool has_wild_user() const            { return get_user() == "*";                             }
	bool has_wild_host() const            { return get_host() == "*";                             }
	bool has_all_wild() const             { return std::string(*this) == "*!*@*";                 }
	bool is_canonical() const;

	// Extended mask utils
	bool has_negation() const             { return has('~');                                      }
	bool has_mask() const                 { return has(':');                                      }
	std::string get_mask() const          { return has_mask()? substr(find(':')+1,npos) : "";     }
	const char &get_exttype() const       { return has_negation()? at(2) : at(1);                 }
	bool has_wild_mask() const            { return get_mask().empty() || get_mask() == "*";       }
	bool is_extended() const              { return size() > 1 && at(0) == '$';                    }

	Form get_form() const;

	bool operator==(const Form &f) const             { return get_form() == f;                    }
	bool operator!=(const Form &f) const             { return get_form() != f;                    }

	bool operator==(const std::string &o) const      { return tolower(*this) == tolower(o);       }
	bool operator!=(const std::string &o) const      { return tolower(*this) != tolower(o);       }
	bool operator<=(const std::string &o) const      { return tolower(*this) <= tolower(o);       }
	bool operator>=(const std::string &o) const      { return tolower(*this) >= tolower(o);       }
	bool operator<(const std::string &o) const       { return tolower(*this) < tolower(o);        }
	bool operator>(const std::string &o) const       { return tolower(*this) > tolower(o);        }

	bool operator==(const Mask &o) const             { return tolower(*this) == tolower(o);       }
	bool operator!=(const Mask &o) const             { return tolower(*this) != tolower(o);       }
	bool operator<=(const Mask &o) const             { return tolower(*this) <= tolower(o);       }
	bool operator>=(const Mask &o) const             { return tolower(*this) >= tolower(o);       }
	bool operator<(const Mask &o) const              { return tolower(*this) < tolower(o);        }
	bool operator>(const Mask &o) const              { return tolower(*this) > tolower(o);        }

	template<class... A> Mask(A&&... a): std::string(std::forward<A>(a)...) {}
};


inline
Mask::Form Mask::get_form()
const
{
	return is_canonical()? CANONICAL:
	       is_extended()?  EXTENDED:
	                       INVALID;
}


inline
bool Mask::is_canonical()
const
{
	return has('!') &&
	       has('@') &&
	       num('!') == 1 &&
	       num('@') == 1;
	       find('!') < find('@') &&
	       !get_nick().empty() &&
	       !get_user().empty() &&
	       !get_host().empty();
}
