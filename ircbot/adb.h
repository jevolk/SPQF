/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Adb
{
	stldb::ldb<std::string,std::string> ldb;

  public:
	template<class... A> auto begin(A&&... a) const      { return ldb.begin(std::forward<A>(a)...); }
	template<class... A> auto begin(A&&... a)            { return ldb.begin(std::forward<A>(a)...); }
	template<class... A> auto end(A&&... a) const        { return ldb.end(std::forward<A>(a)...);   }
	template<class... A> auto end(A&&... a)              { return ldb.end(std::forward<A>(a)...);   }

	bool exists(const std::string &name) const           { return ldb.count(name);                  }
	auto count() const                                   { return ldb.size();                       }

	Adoc get(const std::nothrow_t, const std::string &name) const noexcept;
	Adoc get(const std::nothrow_t, const std::string &name) noexcept;
	Adoc get(const std::string &name) const;
	Adoc get(const std::string &name);

	void set(const std::string &name, const Adoc &data)  { ldb.insert(name,data);                   }

	Adb(const std::string &dir);
};


inline
Adb::Adb(const std::string &dir):
ldb(dir)
{

}


inline
Adoc Adb::get(const std::string &name)
{
	const auto it = ldb.find(name);
	return it? Adoc{it->second} : throw Exception("Account not found");
}


inline
Adoc Adb::get(const std::string &name)
const
{
	const auto it = ldb.find(name);
	return it? Adoc{it->second} : throw Exception("Account not found");
}


inline
Adoc Adb::get(const std::nothrow_t,
              const std::string &name)
noexcept
{
	const auto it = ldb.find(name);
	return it? Adoc{it->second} : Adoc{};
}


inline
Adoc Adb::get(const std::nothrow_t,
              const std::string &name)
const noexcept
{
	const auto it = ldb.find(name);
	return it? Adoc{it->second} : Adoc{};
}
