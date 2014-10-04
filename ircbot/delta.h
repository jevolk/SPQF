/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Delta : std::tuple<bool,char,Mask>
{
	enum Field  { SIGN, MODE, MASK                       };
	enum Valid  { VALID, NOT_FOUND, NEED_MASK, CANT_MASK };

	bool need_mask_chan(const Server &s) const   { return s.chan_pmodes.has(std::get<MODE>(*this)); }
	bool need_mask_user(const Server &s) const   { return s.user_pmodes.has(std::get<MODE>(*this)); }
	bool exists_chan(const Server &s) const      { return s.chan_modes.has(std::get<MODE>(*this));  }
	bool exists_user(const Server &s) const      { return s.user_modes.has(std::get<MODE>(*this));  }

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
	// TODO: Non-mask modes like +l
	if(!mask.empty() && mask.get_form() == Mask::INVALID)
		throw Exception("Mask format is invalid.");
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
std::ostream &operator<<(std::ostream &s,
                         const Delta &delta)
{
	s << (std::get<Delta::SIGN>(delta)? '+' : '-');
	s << std::get<Delta::MODE>(delta);

	if(!std::get<Delta::MASK>(delta).empty())
		s << " " << std::get<Delta::MASK>(delta);

	return s;
}
