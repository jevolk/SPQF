/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Vdb : public Adb
{
  public:
	Vote get(const std::nothrow_t, const Vote::id_t &id) noexcept;
	Vote get(const Vote::id_t &id);

	Vdb(const std::string &dir);
};


inline
Vdb::Vdb(const std::string &dir):
Adb(dir)
{

}


inline
Vote Vdb::get(const Vote::id_t &id)
{
	return {id,*this};
}


inline
Vote Vdb::get(const std::nothrow_t,
              const Vote::id_t &id)
noexcept
{
	return {id,*this};
}
