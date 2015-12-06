/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Vdb : public Adb
{
	using id_t = Vote::id_t;

  public:
	bool exists(const id_t &id) const               { return Adb::exists(lex_cast(id));  }

	template<class T, class... Args> T get(const id_t &id, Args&&... args);
	template<class... Args> std::unique_ptr<Vote> get(const id_t &id, Args&&... args);
	std::string get_type(const id_t &id);

	using Term = std::pair<std::string,std::string>;
	std::list<id_t> find(const std::forward_list<Term> &terms);

	Vdb(const std::string &dir);
};


inline
Vdb::Vdb(const std::string &dir):
Adb(dir)
{
}


inline
std::list<id_t>
Vdb::find(const std::forward_list<Term> &terms)
{
	std::list<id_t> ret;
	const auto num_terms(std::distance(terms.begin(),terms.end()));
	for(auto it(cbegin()); it != cend(); ++it)
	{
		auto match(0);
		const auto &id(lex_cast<id_t>(it->first));
		const Adoc doc{std::string{it->second}};
		for(const auto &term : terms)
		{
			const auto &key(term.first);
			const auto &val(term.second);
			match += tolower(doc[key]) == val;
		}

		if(match == num_terms)
			ret.emplace_back(id);
	}

	return ret;
}


inline
std::string Vdb::get_type(const id_t &id)
{
	return Adb::get(std::nothrow,lex_cast(id))["type"];
}


template<class... Args>
std::unique_ptr<Vote> Vdb::get(const id_t &id,
                               Args&&... args)
try
{
	switch(hash(get_type(id)))
	{
		case hash("config"):    return std::make_unique<vote::Config>(id,*this,std::forward<Args>(args)...);
		case hash("mode"):      return std::make_unique<vote::Mode>(id,*this,std::forward<Args>(args)...);
		case hash("appeal"):    return std::make_unique<vote::Appeal>(id,*this,std::forward<Args>(args)...);
		case hash("kick"):      return std::make_unique<vote::Kick>(id,*this,std::forward<Args>(args)...);
		case hash("invite"):    return std::make_unique<vote::Invite>(id,*this,std::forward<Args>(args)...);
		case hash("topic"):     return std::make_unique<vote::Topic>(id,*this,std::forward<Args>(args)...);
		case hash("opine"):     return std::make_unique<vote::Opine>(id,*this,std::forward<Args>(args)...);
		case hash("quote"):     return std::make_unique<vote::Quote>(id,*this,std::forward<Args>(args)...);
		case hash("ban"):       return std::make_unique<vote::Ban>(id,*this,std::forward<Args>(args)...);
		case hash("quiet"):     return std::make_unique<vote::Quiet>(id,*this,std::forward<Args>(args)...);
		case hash("unquiet"):   return std::make_unique<vote::UnQuiet>(id,*this,std::forward<Args>(args)...);
		case hash("voice"):     return std::make_unique<vote::Voice>(id,*this,std::forward<Args>(args)...);
		case hash("devoice"):   return std::make_unique<vote::DeVoice>(id,*this,std::forward<Args>(args)...);
		case hash("flags"):     return std::make_unique<vote::Flags>(id,*this,std::forward<Args>(args)...);
		case hash("civis"):     return std::make_unique<vote::Civis>(id,*this,std::forward<Args>(args)...);
		case hash("censure"):   return std::make_unique<vote::Censure>(id,*this,std::forward<Args>(args)...);
		case hash("import"):    return std::make_unique<vote::Import>(id,*this,std::forward<Args>(args)...);
		default:                return std::make_unique<Vote>("",id,*this,std::forward<Args>(args)...);
	}
}
catch(const Exception &e)
{
	if(exists(id))
		throw;

	throw Exception("Could not find a vote by that ID.");
}


template<class T,
         class... Args>
T Vdb::get(const id_t &id,
           Args&&... args)
try
{
	return T{"",id,*this,std::forward<Args>(args)...};
}
catch(const Exception &e)
{
	if(exists(id))
		throw;

	throw Exception("Could not find a vote by that ID.");
}
