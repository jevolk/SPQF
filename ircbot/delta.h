/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Delta : std::tuple<bool,char,Mask>
{
	enum Field                                   { SIGN, MODE, MASK                                 };
	enum Valid                                   { VALID, NOT_FOUND, NEED_MASK, CANT_MASK           };

	static char sign(const bool &b)              { return b? '+' : '-';                             }
	static bool sign(const char &s)              { return s != '-';                                 }
	static bool is_sign(const char &s)           { return s == '-' || s == '+';                     }
	static bool needs_inv_mask(const char &m);   // Modes that take an argument which is not a Mask

	bool need_mask_chan(const Server &s) const;
	bool need_mask_user(const Server &s) const;
	bool exists_chan(const Server &s) const;
	bool exists_user(const Server &s) const;
	bool valid_mask() const;

	// returns reason
	Valid check_chan(const Server &s) const;
	Valid check_user(const Server &s) const;

	// false on not VALID
	bool valid_chan(const Server &s) const       { return check_chan(s) == VALID;                   }
	bool valid_user(const Server &s) const       { return check_user(s) == VALID;                   }

	// Throws on not VALID
	void validate_chan(const Server &s) const;
	void validate_user(const Server &s) const;

	operator std::string() const;
	void check() const;                          // Throws why failed

	Delta(const bool &sign, const char &mode, const Mask &mask);
	Delta(const char &sign, const char &mode, const Mask &mask);
	Delta(const std::string &mode_delta, const Mask &mask);
	Delta(const std::string &delta);

	friend std::ostream &operator<<(std::ostream &s, const Delta &delta);
};


struct Deltas : std::vector<Delta>
{
	bool all_signs(const bool &sign) const;
	bool too_many(const Server &s) const         { return size() > s.isupport.get_or_max("MODES");  }
	void validate_chan(const Server &s) const;
	void validate_user(const Server &s) const;

	std::string substr(const const_iterator &begin, const const_iterator &end) const;
	operator std::string() const;

	template<class... T> Deltas &operator<<(T&&... t);

	Deltas() = default;
	Deltas(std::vector<Delta> &&vec):       std::vector<Delta>(std::move(vec)) {}
	Deltas(const std::vector<Delta> &vec):  std::vector<Delta>(vec) {}

	//NOTE: Modes with arguments must only be chan_pmodes for now
	Deltas(const std::string &delts, const Server *const &serv = nullptr);
	Deltas(const std::string &delts, const Server &serv): Deltas(delts,&serv) {}

	friend std::ostream &operator<<(std::ostream &s, const Deltas &d);
};


inline
Deltas::Deltas(const std::string &delts,
               const Server *const &serv)
try
{
	const auto tok = tokens(delts);
	const auto &ms = tok.at(0);
	bool sign = Delta::sign(ms.at(0));
	for(size_t i = 0, a = 1; i < ms.size(); i++)
	{
		if(Delta::is_sign(ms.at(i)))
			sign = Delta::sign(ms.at(i++));

		const char &mode = ms.at(i);
		const bool has_arg = serv? serv->chan_pmodes.find(mode) != std::string::npos : tok.size() > a;
		const auto &arg = has_arg? tok.at(a++) : "";
		emplace_back(sign,mode,arg);
	}
}
catch(const std::out_of_range &e)
{
	throw Exception("Improperly formatted deltas string.");
}


template<class... T>
Deltas &Deltas::operator<<(T&&... t)
{
	emplace_back(std::forward<T>(t)...);
	return *this;
}


inline
void Deltas::validate_user(const Server &s)
const
{
	if(too_many(s))
		throw Exception("Server doesn't support changing this many modes at once.");

	for(const Delta &delta : *this)
		delta.validate_user(s);
}


inline
void Deltas::validate_chan(const Server &s)
const
{
	if(too_many(s))
		throw Exception("Server doesn't support changing this many modes at once.");

	for(const Delta &delta : *this)
		delta.validate_chan(s);
}


inline
bool Deltas::all_signs(const bool &sign)
const
{
	return std::all_of(begin(),end(),[&sign]
	(const Delta &delta)
	{
		return std::get<Delta::SIGN>(delta) == sign;
	});
}


inline
Deltas::operator std::string()
const
{
	return string(*this);
}


inline
std::string Deltas::substr(const Deltas::const_iterator &begin,
                           const Deltas::const_iterator &end)
const
{
	std::stringstream s;

	for(auto it = begin; it != end; ++it)
	{
		const auto &d = *it;
		s << d.sign(std::get<Delta::SIGN>(d)) << std::get<Delta::MODE>(d);
	}

	for(auto it = begin; it != end; ++it)
	{
		const auto &d = *it;
		s << " " << std::get<Delta::MASK>(d);
	}

	return s.str();
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Deltas &deltas)
{
	s << deltas.substr(deltas.begin(),deltas.end());
	return s;
}


inline
Delta::Delta(const std::string &delta)
try:
Delta(delta.substr(0,delta.find(" ")),
      delta.size() > 2? delta.substr(delta.find(" ")+1) : "")
{

}
catch(const std::out_of_range &e)
{
	throw Exception("Improperly formatted delta string (no space?).");
}


inline
Delta::Delta(const std::string &mode_delta,
             const Mask &mask)
try:
Delta(sign(mode_delta.at(0)),
      mode_delta.at(1),
      mask)
{
	if(mode_delta.size() != 2)
		throw Exception("Bad mode delta: Need two characters: [+/-][mode].");
}
catch(const std::out_of_range &e)
{
	throw Exception("Bad mode delta: Need two characters: [+/-][mode] (had less).");
}


inline
Delta::Delta(const char &sc,
             const char &mode,
             const Mask &mask):
Delta(sign(sc),mode,mask)
{

}


inline
Delta::Delta(const bool &sign,
             const char &mode,
             const Mask &mask):
std::tuple<bool,char,Mask>(sign,mode,mask)
{

}


inline
void Delta::check()
const
{
	if(!std::get<MASK>(*this).empty())
	{
		if(needs_inv_mask(std::get<MODE>(*this)) && valid_mask())
			throw Exception("Mode does not require a hostmask argument.");
	}
}


inline
void Delta::validate_user(const Server &s)
const
{
	switch(check_user(s))
	{
		case NOT_FOUND:       throw Exception("Mode is not valid for users on this server.");
		case NEED_MASK:       throw Exception("Mode requries an argument.");
		case CANT_MASK:       throw Exception("Mode does not have an argument.");
		case VALID:
		default:              return;
	};
}


inline
void Delta::validate_chan(const Server &s)
const
{
	switch(check_chan(s))
	{
		case NOT_FOUND:       throw Exception("Mode is not valid for channels on this server.");
		case NEED_MASK:       throw Exception("Mode requries an argument.");
		case CANT_MASK:       throw Exception("Mode does not have an argument.");
		case VALID:
		default:              return;
	};
}


inline
Delta::Valid Delta::check_user(const Server &s)
const
{
	if(!exists_user(s))
		return NOT_FOUND;

	if(std::get<MASK>(*this).empty() && need_mask_user(s))
		return NEED_MASK;

	if(!std::get<MASK>(*this).empty() && !need_mask_user(s))
		return CANT_MASK;

	return VALID;
}

inline
Delta::Valid Delta::check_chan(const Server &s)
const
{
	if(!exists_chan(s))
		return NOT_FOUND;

	if(std::get<MASK>(*this).empty() && need_mask_chan(s))
		return NEED_MASK;

	if(!std::get<MASK>(*this).empty() && !need_mask_chan(s))
		return CANT_MASK;

	return VALID;
}


inline
bool Delta::valid_mask()
const
{
	return std::get<MASK>(*this).get_form() != Mask::INVALID;
}


inline
bool Delta::need_mask_chan(const Server &s)
const
{
	return s.chan_pmodes.find(std::get<MODE>(*this)) != std::string::npos;
}


inline
bool Delta::need_mask_user(const Server &s)
const
{
	return s.user_pmodes.find(std::get<MODE>(*this)) != std::string::npos;
}


inline
bool Delta::exists_chan(const Server &s)
const
{
	return s.chan_modes.find(std::get<MODE>(*this)) != std::string::npos;
}


inline
bool Delta::exists_user(const Server &s)
const
{
	return s.user_modes.find(std::get<MODE>(*this)) != std::string::npos;
}


inline
bool Delta::needs_inv_mask(const char &m)
{
	switch(m)
	{
		case 'o':
		case 'v':
		case 'l':
		case 'j':
			return true;

		default:
			return false;
	}
}


inline
Delta::operator std::string()
const
{
	return string(*this);
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Delta &d)
{
	s << d.sign(std::get<Delta::SIGN>(d));
	s << std::get<Delta::MODE>(d);

	if(!std::get<Delta::MASK>(d).empty())
		s << " " << std::get<Delta::MASK>(d);

	return s;
}
