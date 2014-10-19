/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Vdb : Adb
{
	class Iterator : public Ldb::Iterator
	{
		bool valid() const override;

	  public:
		template<class... Args> Iterator(Vdb &vdb, const size_t &id = 0, Args&&... args);
	};

	auto exists(const size_t &id) const            { return Adb::exists(lex_cast(id));          }
	uint count() const;

	Vdb(Adb &adb): Adb(adb.get_ldb()) {}
};


template<class... Args>
Vdb::Iterator::Iterator(Vdb &vdb,
                        const size_t &id,
                        Args&&... args):
Ldb::Iterator(vdb.get_ldb(),
              lex_cast(id),
              false,
              std::forward<Args>(args)...)
{

}


inline
uint Vdb::count()
const
{
	Iterator it(const_cast<Vdb &>(*this));
	return it.count();
}


inline
bool Vdb::Iterator::valid()
const
{
	if(!it->Valid())
		return false;

	const auto &key = it->key();
	return isnumeric(key.data(),key.data()+key.size());
}
