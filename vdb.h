/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Vdb : Adb
{
	auto exists(const size_t &id) const              { return Adb::exists(lex_cast(id));      }
	uint count() const;

	Vdb(Adb &adb): Adb(adb.get_ldb()) {}
};


inline
uint Vdb::count()
const
{

	return 0;
}

/*
inline
bool Vdb::Iterator::valid()
const
{
	if(!it->Valid())
		return false;

	const auto &key = it->key();
	return isnumeric(key.data(),key.data()+key.size());
}
*/
