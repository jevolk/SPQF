/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Vdb : Adb
{
	using Term = std::tuple<std::string,std::string,std::string>;
	using Terms = std::forward_list<Term>;
	using Results = std::list<id_t>;

	static const std::vector<std::string> operators;

	bool exists(const id_t &id) const;
	std::unique_ptr<Vote> get(const id_t &id);
	std::string get_value(const id_t &id, const std::string &key);
	std::string get_type(const id_t &id);

  private:
	static bool match(const Adoc &doc, const Term &term);
	template<class It> static void query(const Terms &terms, const size_t &limit, It&& begin, It&& end, Results &ret);

  public:
	Results query(const Terms &terms, const size_t &limit = 0, const bool &descending = true);

	Vdb(const std::string &dir);
};


template<class It>
void Vdb::query(const Terms &terms,
                const size_t &limit,
                It&& begin,
                It&& end,
                Results &ret)
{
	size_t rets(0);
	for(auto it(begin); it != end && (!limit || rets < limit); ++it)
	{
		const auto &id(lex_cast<id_t>(it->first));
		const Adoc doc(it->second);
		const auto matched(std::all_of(terms.begin(),terms.end(),[&doc]
		(const auto &term)
		{
			return match(doc,term);
		}));

		if(matched)
		{
			ret.emplace_back(id);
			++rets;
		}
	}
}
