/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Adb
{
	Ldb &ldb;

  public:
	const Ldb &get_ldb() const                           { return ldb;                              }
	Ldb &get_ldb()                                       { return ldb;                              }

	bool exists(const std::string &name) const           { return ldb.exists(name);                 }
	auto count() const                                   { return ldb.count();                      }

	Adoc get(const std::nothrow_t, const std::string &name) const noexcept;
	Adoc get(const std::nothrow_t, const std::string &name) noexcept;
	Adoc get(const std::string &name) const;
	Adoc get(const std::string &name);

	void set(const std::string &name, const Adoc &data)  { ldb.set(name,data);                      }

	Adb(Ldb &ldb);
};


inline
Adb::Adb(Ldb &ldb):
ldb(ldb)
{

}


inline
Adoc Adb::get(const std::string &name)
{
	const auto it = ldb.find(name);

	if(!it)
		throw Exception("Account not found");

	return std::string(std::get<decltype(it)::VAL>(*it),std::get<decltype(it)::VAL_SIZE>(*it));
}


inline
Adoc Adb::get(const std::string &name)
const
{
	const auto it = ldb.find(name);

	if(!it)
		throw Exception("Account not found");

	return std::string(std::get<decltype(it)::VAL>(*it),std::get<decltype(it)::VAL_SIZE>(*it));
}


inline
Adoc Adb::get(const std::nothrow_t,
              const std::string &name)
noexcept
{
	const auto it = ldb.find(name);
	return it? std::string(std::get<decltype(it)::VAL>(*it),std::get<decltype(it)::VAL_SIZE>(*it)):
	           std::string();
}


inline
Adoc Adb::get(const std::nothrow_t,
              const std::string &name)
const noexcept
{
	const auto it = ldb.find(name);
	return it? std::string(std::get<decltype(it)::VAL>(*it),std::get<decltype(it)::VAL_SIZE>(*it)):
	           std::string();
}
