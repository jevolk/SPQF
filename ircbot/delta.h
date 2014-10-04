/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Delta : std::tuple<bool,char,Mask>
{
	enum { SIGN, MODE, MASK };

	bool needs_mask_chan(const Server &s) const  { return s.chan_pmodes.has(std::get<MODE>(*this)); }
	bool needs_mask_user(const Server &s) const  { return s.user_pmodes.has(std::get<MODE>(*this)); }
	bool exists_chan(const Server &s) const      { return s.chan_modes.has(std::get<MODE>(*this));  }
	bool exists_user(const Server &s) const      { return s.user_modes.has(std::get<MODE>(*this));  }

	bool valid_chan(const Server &s) const
	{
		return exists_chan(s) &&
		       ((std::get<MASK>(*this).empty() && !needs_mask_chan(s)) ||
		       (!std::get<MASK>(*this).empty() && needs_mask_chan(s)));
	}

	bool valid_user(const Server &s) const
	{
		return exists_user(s) &&
		       ((std::get<MASK>(*this).empty() && !needs_mask_user(s)) ||
		       (!std::get<MASK>(*this).empty() && needs_mask_user(s)));
	}

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
	throw Exception("Improperly formatted delta string (no space?)");
}


inline
Delta::Delta(const std::string &mode_delta,
             const Mask &mask)
try:
Delta((mode_delta.at(0) == '+'? true:
       mode_delta.at(0) == '-'? false:
       throw Exception("Invalid +/- sign character")),
      mode_delta.at(1),
      mask)
{
	if(mode_delta.size() != 2)
		throw Exception("Bad mode delta: Need two characters: [+/-][mode]");
}
catch(const std::out_of_range &e)
{
	throw Exception("Bad mode delta: Need two characters: [+/-][mode] (had less)");
}


inline
Delta::Delta(const bool &sign,
             const char &mode,
             const Mask &mask):
std::tuple<bool,char,Mask>(sign,mode,mask)
{
	if(!mask.empty() && mask.get_form() == Mask::INVALID)
		throw Exception("Mask format is invalid");
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
