/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Vdb
{
	Adb &adb;

  public:
	class Iterator : public Ldb::Iterator
	{
		bool valid() const override;

	  public:
		template<class... A> Iterator(Vdb &vdb, A&&... a);
	};

	auto has_vote(const std::string &id) const       { return adb.exists(id);                   }
	auto has_vote(const id_t &id) const              { return adb.exists(lex_cast(id));         }
	uint num_votes() const;

	Vdb(Adb &adb): adb(adb) {}
};


template<class... A>
Vdb::Iterator::Iterator(Vdb &vdb,
                        A&&... a):
Ldb::Iterator(vdb.adb.get_ldb(),std::forward<A>(a)...)
{

}


inline
uint Vdb::num_votes()
const
{
	Iterator it(const_cast<Vdb &>(*this));
	return it.count();
}


inline
bool Vdb::Iterator::valid()
const
{
	if(!Ldb::Iterator::valid())
		return false;

	const auto &key = it->key();
	return isnumeric(key.data(),key.data()+key.size());
}
