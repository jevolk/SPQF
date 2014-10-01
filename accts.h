/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Acct : public boost::property_tree::ptree
{
	operator std::string() const;

	Acct() = default;
	Acct(const std::string &str);

	friend std::ostream &operator<<(std::ostream &s, const Acct &acct);
};


class Accts
{
	Ldb ldb;

  public:
	bool exists(const std::string &name) const         { return ldb.exists(name);                   }
	size_t count() const                               { return ldb.count();                        }

	Acct get(const std::nothrow_t, const std::string &name) const noexcept;
	Acct get(const std::nothrow_t, const std::string &name) noexcept;
	Acct get(const std::string &name) const;
	Acct get(const std::string &name);

	void set(const std::string &name, const Acct &data);

	Accts(const std::string &dir);
};


inline
Accts::Accts(const std::string &dir):
ldb(dir)
{


}


inline
void Accts::set(const std::string &name,
                const Acct &data)
{
	ldb.set(name,data);
}


inline
Acct Accts::get(const std::string &name)
{
	const Acct ret = get(std::nothrow,name);

	if(ret.empty())
		throw Exception("Account not found");

	return ret;
}


inline
Acct Accts::get(const std::string &name)
const
{
	const Acct ret = get(std::nothrow,name);

	if(ret.empty())
		throw Exception("Account not found");

	return ret;
}


inline
Acct Accts::get(const std::nothrow_t,
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
Acct Accts::get(const std::nothrow_t,
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
Acct::Acct(const std::string &str)
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
Acct::operator std::string()
const
{
	std::stringstream ss;
	ss << (*this);
	return ss.str();
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Acct &acct)
{
	boost::property_tree::write_json(s,acct);
	return s;
}
