/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Adoc : public boost::property_tree::ptree
{
	operator std::string() const;

	bool has(const std::string &key) const               { return !get(key,std::string()).empty();  }
	bool has_child(const std::string &key) const         { return count(key) > 0;                   }
	auto operator[](const std::string &key) const        { return get(key,std::string());           }

	// Array document utils
	template<class C> C into() const;
	template<class C> C into(const C &) const            { return into<C>();                        }

	template<class T> Adoc &push(const T &val) &;
	template<class I> Adoc &push(const I &beg, const I &end) &;

	bool remove(const std::string &key) &;
	Adoc &merge(const Adoc &src) &;                      // src takes precedence over this

	Adoc(const std::string &str = "{}");
	Adoc(boost::property_tree::ptree &&p):               boost::property_tree::ptree(std::move(p)) {}
	Adoc(const boost::property_tree::ptree &p):          boost::property_tree::ptree(p) {}

	friend std::ostream &operator<<(std::ostream &s, const Adoc &adoc);
};


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
Adoc &Adoc::merge(const Adoc &src)
&
{
	const std::function<void (const Adoc &src, Adoc &dst)> recurse = [&recurse]
	(const Adoc &src, Adoc &dst)
	{
		for(const auto &pair : src)
		{
			const auto &key = pair.first;
			const auto &sub = pair.second;

			if(!sub.empty())
			{
				Adoc child = dst.get_child(key,Adoc());
				recurse(sub,child);
				dst.put_child(key,child);
			}
			else dst.put_child(key,sub);
		}
	};

	recurse(src,*this);
	return *this;
}


inline
bool Adoc::remove(const std::string &key)
&
{
	const size_t pos = key.rfind(".");
	if(pos == std::string::npos)
		return erase(key);

	const std::string path = key.substr(0,pos);
	const std::string skey = key.substr(pos+1);

	if(skey.empty())
		erase(path);

	auto &doc = get_child(path);
	const bool ret = doc.erase(skey);

	if(doc.empty())
		erase(path);

	return ret;
}


template<class I>
Adoc &Adoc::push(const I &beg,
                 const I &end)
&
{
	std::for_each(beg,end,[this]
	(const auto &val)
	{
		this->push(val);
	});

	return *this;
}


template<class T>
Adoc &Adoc::push(const T &val)
&
{
	Adoc tmp;
	tmp.put("",val);
	push_back({"",tmp});
	return *this;
}


template<class C>
C Adoc::into()
const
{
	C ret;
	std::transform(begin(),end(),std::inserter(ret,ret.begin()),[]
	(const auto &p)
	{
		return p.second.get<typename C::value_type>("");
	});

	return ret;
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
