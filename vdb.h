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
	bool exists(const id_t &id) const    { return Adb::exists(boost::lexical_cast<std::string>(id)); }

	template<class... Args> Vote get(const id_t &id, Args&&... args);

	Vdb(const std::string &dir);
};


inline
Vdb::Vdb(const std::string &dir):
Adb(dir)
{

}


template<class... Args>
Vote Vdb::get(const id_t &id,
              Args&&... args)
try
{
	return {id,*this,std::forward<Args>(args)...};
}
catch(const Exception &e)
{
	if(exists(id))
		throw;

	throw Exception("Could not find a vote by that ID.");
}
