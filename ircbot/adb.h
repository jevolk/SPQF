/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Adoc : public boost::property_tree::ptree
{
	operator std::string() const;

	Adoc() = default;
	Adoc(const std::string &str);
	Adoc(boost::property_tree::ptree &&p):       boost::property_tree::ptree(std::move(p)) {}
	Adoc(const boost::property_tree::ptree &p):  boost::property_tree::ptree(p) {}

	friend std::ostream &operator<<(std::ostream &s, const Adoc &adoc);
};


class Adb
{
	Ldb ldb;

  public:
	bool exists(const std::string &name) const          { return ldb.exists(name);                 }
	size_t count() const                                { return ldb.count();                      }

	Adoc get(const std::nothrow_t, const std::string &name) const noexcept;
	Adoc get(const std::nothrow_t, const std::string &name) noexcept;
	Adoc get(const std::string &name) const;
	Adoc get(const std::string &name);

	void set(const std::string &name, const Adoc &data);

	Adb(const std::string &dir);
};


class Acct
{
	Adb &adb;
	const std::string &acct;      // the document key in the database; subclass holds data

  public:
	const std::string &get_acct() const                 { return acct;                             }
	bool has_acct() const                               { return adb.exists(get_acct());           }

	// Get documents
	Adoc get(const std::string &key) const              { return adb.get(std::nothrow,get_acct()); }
	Adoc get(const std::string &key)                    { return adb.get(std::nothrow,get_acct()); }
	Adoc operator[](const std::string &key) const       { return get(key);                         }
	Adoc operator[](const std::string &key)             { return get(key);                         }
	bool has(const std::string &key) const              { return !get(key).empty();                }

	// Set documents
	void set(const std::string &key, const Adoc &doc);

	// Convenience for single key => value
	template<class T = std::string> T getval(const std::string &key) const;
	template<class T = std::string> T getval(const std::string &key);
	template<class T> void setval(const std::string &key, const T &t);

  protected:
	Acct(Adb &adb, const std::string &acct);
};


inline
Acct::Acct(Adb &adb,
           const std::string &acct):
adb(adb),
acct(acct)
{

}


template<class T>
void Acct::setval(const std::string &key,
                  const T &t)
{
	Adoc doc = adb.get(std::nothrow,get_acct());
	doc.put(key,t);
	set(key,doc);
}


inline
void Acct::set(const std::string &key,
               const Adoc &doc)
{
	adb.set(get_acct(),doc);
}


template<class T>
T Acct::getval(const std::string &key)
{
	const Adoc doc = get(key);
	return doc.get<T>(key);
}


template<class T>
T Acct::getval(const std::string &key)
const
{
	const Adoc doc = get(key);
	return doc.get<T>(key);
}



inline
Adb::Adb(const std::string &dir):
ldb(dir)
{

}


inline
void Adb::set(const std::string &name,
              const Adoc &data)
{
	ldb.set(name,data);
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



inline
Adoc::Adoc(const std::string &str)
try:
boost::property_tree::ptree([&str]
{
	std::stringstream ss(str);
	boost::property_tree::ptree ret;
	boost::property_tree::json_parser::read_json(ss,ret);
	return ret;
}())
{

}
catch(const boost::property_tree::json_parser::json_parser_error &e)
{
	throw Exception(e.what());
}


inline
Adoc::operator std::string()
const
{
	std::stringstream ss;
	ss << (*this);
	return ss.str();
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Adoc &doc)
{
	boost::property_tree::write_json(s,doc);
	return s;
}
