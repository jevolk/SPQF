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
	const Adoc ret = get(std::nothrow,name);
	if(ret.empty())
		throw Exception("Account not found");

	return ret;
}


inline
Adoc Adb::get(const std::string &name)
const
{
	const Adoc ret = get(std::nothrow,name);
	if(ret.empty())
		throw Exception("Account not found");

	return ret;
}


inline
Adoc Adb::get(const std::nothrow_t,
              const std::string &name)
noexcept try
{
	return ldb.get(std::nothrow,name);
}
catch(const Exception &e)
{
	return {};
}


inline
Adoc Adb::get(const std::nothrow_t,
              const std::string &name)
const noexcept try
{
	return ldb.get(std::nothrow,name);
}
catch(const Exception &e)
{
	return {};
}
