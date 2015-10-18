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

	Vdb(const std::string &dir);
};


inline
Vdb::Vdb(const std::string &dir):
Adb(dir)
{

}


inline
std::string Vdb::get_type(const id_t &id)
{
	const auto doc = Adb::get(std::nothrow,lex_cast(id));
	return doc["type"];
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
		case hash("kick"):      return std::make_unique<vote::Kick>(id,*this,std::forward<Args>(args)...);
		case hash("invite"):    return std::make_unique<vote::Invite>(id,*this,std::forward<Args>(args)...);
		case hash("topic"):     return std::make_unique<vote::Topic>(id,*this,std::forward<Args>(args)...);
		case hash("opine"):     return std::make_unique<vote::Opine>(id,*this,std::forward<Args>(args)...);
		case hash("ban"):       return std::make_unique<vote::Ban>(id,*this,std::forward<Args>(args)...);
		case hash("quiet"):     return std::make_unique<vote::Quiet>(id,*this,std::forward<Args>(args)...);
		case hash("unquiet"):   return std::make_unique<vote::UnQuiet>(id,*this,std::forward<Args>(args)...);
		case hash("voice"):     return std::make_unique<vote::Voice>(id,*this,std::forward<Args>(args)...);
		case hash("devoice"):   return std::make_unique<vote::DeVoice>(id,*this,std::forward<Args>(args)...);
		case hash("flags"):     return std::make_unique<vote::Flags>(id,*this,std::forward<Args>(args)...);
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
