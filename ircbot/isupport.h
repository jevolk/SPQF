/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct ISupport : std::map<std::string,std::string>
{
	bool empty(const std::string &key) const         { return !count(key) || at(key).empty();         }
	bool operator()(const std::string &key) const    { return count(key);                             }

	const std::string &operator[](const std::string &key) const;                        // empty static string

	template<class T = std::string> T get(const std::string &key) const;                // throws
	template<class T = std::string> T get(const std::string &key, T&& def_val) const;   // defaults

	template<class T = size_t> T get_or_max(const std::string &key) const;              // numeric lim
	template<class... A> bool find_in(const std::string &key, A&&... val) const;

	template<class... A> ISupport(A&&... a): std::map<std::string,std::string>(std::forward<A>(a)...) {}
};


template<class... A>
bool ISupport::find_in(const std::string &key,
                       A&&... val)
const
{
	const auto it = find(key);
	if(it == end())
		return false;

	return it->second.find(std::forward<A>(val)...) != std::string::npos;
}


template<class T>
T ISupport::get_or_max(const std::string &key)
const
{
	return get(key,std::numeric_limits<T>::max());
}


template<class T>
T ISupport::get(const std::string &key,
                T&& def_val)
const
{
	return empty(key)? def_val : get<T>(key);
}


template<class T>
T ISupport::get(const std::string &key)
const
{
	return lex_cast<T>(at(key));
}


inline
const std::string &ISupport::operator[](const std::string &key)
const try
{
	return at(key);
}
catch(const std::out_of_range &e)
{
	static const std::string mt;
	return mt;
}
