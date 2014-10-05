/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Delta : std::tuple<bool,char,Mask>
{
	enum Field                                   { SIGN, MODE, MASK                                 };
	enum Valid                                   { VALID, NOT_FOUND, NEED_MASK, CANT_MASK           };

	static bool sign(const char &s);             // throws on not + or -
	static bool needs_inv_mask(const char &m);   // Modes that take an argument which is not a Mask

	bool need_mask_chan(const Server &s) const   { return s.chan_pmodes.has(std::get<MODE>(*this)); }
	bool need_mask_user(const Server &s) const   { return s.user_pmodes.has(std::get<MODE>(*this)); }
	bool exists_chan(const Server &s) const      { return s.chan_modes.has(std::get<MODE>(*this));  }
	bool exists_user(const Server &s) const      { return s.user_modes.has(std::get<MODE>(*this));  }
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

	Delta(const bool &sign, const char &mode, const Mask &mask);
	Delta(const std::string &mode_delta, const Mask &mask);
	Delta(const std::string &delta);

	friend std::ostream &operator<<(std::ostream &s, const Delta &delta);
};


struct Deltas : std::vector<Delta>
{
	bool too_many(const Server &s) const;
	void validate_chan(const Server &s) const;
	void validate_user(const Server &s) const;

	Deltas() = default;
	Deltas(std::vector<Delta> &&vec):       std::vector<Delta>(std::move(vec)) {}
	Deltas(const std::vector<Delta> &vec):  std::vector<Delta>(vec) {}

	Deltas(const std::string &delts);
	Deltas(const std::string &delts, const Server &serv);  // Note: Arg testing for chan only

	friend std::ostream &operator<<(std::ostream &s, const Deltas &d);
};


inline
Deltas::Deltas(const std::string &delts)
try
{
	using delim = boost::char_separator<char>;

	static const delim sep(" ");
	const boost::tokenizer<delim> tokens(delts,sep);
	const std::vector<std::string> tok(tokens.begin(),tokens.end());

	const bool sign = Delta::sign(tok.at(0).at(0));
	for(size_t i = 1; i < tok.size(); i++)
	{
		const char &mode = tok.at(0).at(i);
		const std::string &arg = tok.size() > i? tok.at(i) : "";
		emplace_back(sign,mode,arg);
	}
}
catch(const std::out_of_range &e)
{
	throw Exception("Improperly formatted deltas string.");
}


inline
Deltas::Deltas(const std::string &delts,
               const Server &serv)
try
{
	using delim = boost::char_separator<char>;

	static const delim sep(" ");
	const boost::tokenizer<delim> tokens(delts,sep);
	const std::vector<std::string> tok(tokens.begin(),tokens.end());

	const bool sign = Delta::sign(tok.at(0).at(0));
	for(size_t i = 1, a = 1; i < tok.at(0).size(); i++)
	{
		const char &mode = tok.at(0).at(i);
		if(serv.chan_pmodes.has(mode))
			emplace_back(sign,mode,tok.at(a++));
		else
			emplace_back(sign,mode,"");
	}

	validate_chan(serv);
}
catch(const std::out_of_range &e)
{
	throw Exception("Not enough arguments for this mode string.");
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
bool Deltas::too_many(const Server &s)
const
{
	const std::string &max_modes_s = s.cfg.at("MODES");
	const size_t max_modes = boost::lexical_cast<size_t>(max_modes_s);
	return size() > max_modes;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Deltas &d)
{
	for(const Delta &delta : d)
		s << "[" << delta << "]";

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
Delta((mode_delta.at(0) == '+'? true:
       mode_delta.at(0) == '-'? false:
       throw Exception("Invalid +/- sign character.")),
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
Delta::Delta(const bool &sign,
             const char &mode,
             const Mask &mask):
std::tuple<bool,char,Mask>(sign,mode,mask)
{
	if(!mask.empty())
	{
		if(needs_inv_mask(mode) && valid_mask())
			throw Exception("Mode does not require a hostmask argument.");

		if(!needs_inv_mask(mode) && !valid_mask())
			throw Exception("Mode requires a valid hostmask argument.");
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
bool Delta::needs_inv_mask(const char &m)
{
	switch(m)
	{
		case 'o':
		case 'v':
		case 'l':
			return true;

		default:
			return false;
	}
}


inline
bool Delta::sign(const char &s)
{
	return s == '+'? true:
	       s == '-'? false:
	                 throw Exception("Invalid +/- sign character.");
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Delta &delta)
{
	s << (std::get<Delta::SIGN>(delta)? '+' : '-');
	s << std::get<Delta::MODE>(delta);

	if(!std::get<Delta::MASK>(delta).empty())
		s << " " << std::get<Delta::MASK>(delta);

	return s;
}
