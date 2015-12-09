/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Vdb : Adb
{
	using id_t = Vote::id_t;
	using Term = std::tuple<std::string,std::string,std::string>;
	using Terms = std::forward_list<Term>;
	using Results = std::list<id_t>;
	static constexpr auto operators =
	{
		"",     // empty operator checks if key exists (and is non-empty)
		"=",    // equality operator aliased to ==
		"==",   // case insensitive string equality
		"!=",   // case insensitive string inequality
		"<",    // integer less than
		">",    // integer greater than
		"<=",   // integer less than or equals
		">=",   // integer greater than or equals
	};

	bool exists(const id_t &id) const;
	template<class T, class... Args> T get(const id_t &id, Args&&... args);
	template<class... Args> std::unique_ptr<Vote> get(const id_t &id, Args&&... args);
	std::string get_value(const id_t &id, const std::string &key);
	std::string get_type(const id_t &id);

  private:
	static bool match(const Adoc &doc, const Term &term);
	template<class It> static void query(const Terms &terms, const size_t &limit, It&& begin, It&& end, Results &ret);

  public:
	Results query(const Terms &terms, const size_t &limit = 0, const bool &descending = true);

	Vdb(const std::string &dir);
};


inline
Vdb::Vdb(const std::string &dir):
Adb(dir)
{
}


inline
Vdb::Results Vdb::query(const Terms &terms,
                        const size_t &limit,
                        const bool &descending)
{
	Results ret;

	// TODO: Until stldb reverse iterators and propert sorting are fixed hack this for now...
	query(terms,0,cbegin(),cend(),ret);
	ret.sort();

	if(descending)
		ret.reverse();

	if(limit)
		ret.resize(std::min(limit,ret.size()));

	return ret;
}


template<class It>
void Vdb::query(const Terms &terms,
                const size_t &limit,
                It&& begin,
                It&& end,
                Results &ret)
{
	size_t rets(0);
	for(auto it(begin); it != end && (!limit || rets < limit); ++it)
	{
		const auto &id(lex_cast<id_t>(it->first));
		const Adoc doc(it->second);
		const auto matched(std::all_of(terms.begin(),terms.end(),[&doc]
		(const auto &term)
		{
			return match(doc,term);
		}));

		if(matched)
		{
			ret.emplace_back(id);
			++rets;
		}
	}
}


inline
bool Vdb::match(const Adoc &doc,
                const Term &term)
try
{
	const auto &key(std::get<0>(term));
	const auto &op(std::get<1>(term));
	const auto &val(std::get<2>(term));

	switch(hash(op))
	{
		case hash("="):
		case hash("=="):  return boost::iequals(doc[key],val);
		case hash("!="):  return !boost::iequals(doc[key],val);
		case hash("<="):  return doc.get<int64_t>(key) <= lex_cast<int64_t>(val);
		case hash(">="):  return doc.get<int64_t>(key) >= lex_cast<int64_t>(val);
		case hash("<"):   return doc.get<int64_t>(key) < lex_cast<int64_t>(val);
		case hash(">"):   return doc.get<int64_t>(key) > lex_cast<int64_t>(val);
		case hash(""):    return doc.has(key);
		default:          throw Assertive("Unrecognized query term operator");
	};
}
catch(const boost::bad_lexical_cast &e)
{
	throw Exception("This comparison operator requires a number.");
}
catch(const boost::property_tree::ptree_bad_path &e)
{
	throw Exception("A key is not recognized: ") << e.what();
}
catch(const boost::property_tree::ptree_bad_data &e)
{
	throw Exception("A key can not be compared to the argument: ") << e.what();
}
catch(const boost::property_tree::ptree_error &e)
{
	throw Assertive(e.what());
}


inline
std::string Vdb::get_type(const id_t &id)
{
	return get_value(id,"type");
}


inline
std::string Vdb::get_value(const id_t &id,
                           const std::string &key)
{
	return Adb::get(std::nothrow,lex_cast(id))[key];
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


inline
bool Vdb::exists(const id_t &id)
const
{
	return Adb::exists(lex_cast(id));
}
