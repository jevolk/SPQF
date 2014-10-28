/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Acct
{
	Adb *adb;
	const std::string *acct;                             // the document key ; subclass holds data

  protected:
	auto &get_adb()                                     { return *adb;                              }

  public:
	auto &get_adb() const                               { return *adb;                              }
	auto &get_acct() const                              { return *acct;                             }
	bool exists() const                                 { return adb->exists(get_acct());           }

	// Get document
	Adoc get() const                                    { return adb->get(std::nothrow,get_acct()); }
	Adoc get()                                          { return adb->get(std::nothrow,get_acct()); }
	Adoc get(const std::string &key) const              { return get().get_child(key,Adoc());       }
	Adoc get(const std::string &key)                    { return get().get_child(key,Adoc());       }
	Adoc operator[](const std::string &key) const       { return get(key);                          }
	Adoc operator[](const std::string &key)             { return get(key);                          }

	// Get value of document
	template<class T = std::string> T get_val(const std::string &key) const;
	template<class T = std::string> T get_val(const std::string &key);

	// Check if document exists
	bool has(const std::string &key) const              { return !get_val(key).empty();             }

	// Set document
	void set(const std::string &key, const Adoc &doc);
	void set(const Adoc &doc)                           { adb->set(get_acct(),doc);                 }

	// Convenience for single key => value
	template<class T> void set_val(const std::string &key, const T &t);

	Acct(Adb *const &adb, const std::string *const &acct): adb(adb), acct(acct) {}
	Acct(Adb &adb, const std::string *const &acct): Acct(&adb,acct) {}
	virtual ~Acct() = default;
};


template<class T>
void Acct::set_val(const std::string &key,
                   const T &t)
{
	Adb &adb = get_adb();
	Adoc doc = adb.get(std::nothrow,get_acct());
	doc.put(key,t);
	set(doc);
}


inline
void Acct::set(const std::string &key,
               const Adoc &doc)
{
	Adb &adb = get_adb();
	Adoc main = get();
	main.put_child(key,doc);
	adb.set(get_acct(),main);
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
