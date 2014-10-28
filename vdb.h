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
	template<class... Args> Vote get(const id_t &id, Args&&... args);
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
	const auto doc = Adb::get(lex_cast(id));
	return doc["type"];
}


template<class T,
         class... Args>
T Vdb::get(const id_t &id,
           Args&&... args)
try
{
	return T{id,*this,std::forward<Args>(args)...};
}
catch(const Exception &e)
{
	if(exists(id))
		throw;

	throw Exception("Could not find a vote by that ID.");
}


template<class... Args>
Vote Vdb::get(const id_t &id,
              Args&&... args)
try
{
	return {std::string(),id,*this,std::forward<Args>(args)...};
}
catch(const Exception &e)
{
	if(exists(id))
		throw;

	throw Exception("Could not find a vote by that ID.");
}
