/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Adoc : public boost::property_tree::ptree
{
	operator std::string() const;

	std::string operator[](const std::string &key) const { return get(key,std::string());           }
	bool has_child(const std::string &key) const         { return count(key) > 0;                   }
	bool has(const std::string &key) const               { return !get(key,std::string()).empty();  }

	Adoc(const std::string &str = "{}");
	Adoc(boost::property_tree::ptree &&p):           boost::property_tree::ptree(std::move(p)) {}
	Adoc(const boost::property_tree::ptree &p):      boost::property_tree::ptree(p) {}

	friend std::ostream &operator<<(std::ostream &s, const Adoc &adoc);
};


class Adb
{
	Ldb ldb;

  public:
	bool exists(const std::string &name) const       { return ldb.exists(name);                     }
	size_t count() const                             { return ldb.count();                          }

	Adoc get(const std::nothrow_t, const std::string &name) const noexcept;
	Adoc get(const std::nothrow_t, const std::string &name) noexcept;
	Adoc get(const std::string &name) const;
	Adoc get(const std::string &name);

	void set(const std::string &name, const Adoc &data);

	Adb(const std::string &dir);
	Adb(const Adb &) = delete;
	Adb &operator=(const Adb &) = delete;
};


class Acct
{
	Adb &adb;
	const std::string &acct;      // the document key in the database; subclass holds data

  public:
	const std::string &get_acct() const              { return acct;                                 }
	bool has_acct() const                            { return adb.exists(get_acct());               }

	// Get document
	Adoc get() const                                 { return adb.get(std::nothrow,get_acct());     }
	Adoc get()                                       { return adb.get(std::nothrow,get_acct());     }
	Adoc get(const std::string &key) const           { return get().get_child(key,Adoc());          }
	Adoc get(const std::string &key)                 { return get().get_child(key,Adoc());          }
	Adoc operator[](const std::string &key) const    { return get(key);                             }
	Adoc operator[](const std::string &key)          { return get(key);                             }

	// Get value of document
	template<class T = std::string> T get_val(const std::string &key) const;
	template<class T = std::string> T get_val(const std::string &key);

	// Check if document exists
	bool has(const std::string &key) const           { return !get_val(key).empty();                }

	// Set document
	void set(const Adoc &doc);

	// Convenience for single key => value
	template<class T> void set_val(const std::string &key, const T &t);


  protected:
	Acct(Adb &adb, const std::string &acct);
	virtual ~Acct() = default;
};


inline
Acct::Acct(Adb &adb,
           const std::string &acct):
adb(adb),
acct(acct)
{

}


template<class T>
void Acct::set_val(const std::string &key,
                   const T &t)
{
	Adoc doc = adb.get(std::nothrow,get_acct());
	doc.put(key,t);
	set(doc);
}


inline
void Acct::set(const Adoc &doc)
{
	adb.set(get_acct(),doc);
}


template<class T>
T Acct::get_val(const std::string &key)
{
	const Adoc doc = get(key);
	return doc.get_value<T>(T());
}


template<class T>
T Acct::get_val(const std::string &key)
const
{
	const Adoc doc = get(key);
	return doc.get_value<T>(T());
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
	boost::property_tree::write_json(s,doc,false);
	return s;
}
