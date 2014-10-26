/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


template<class Arg>
class OpDo
{
	Deltas deltas;
	std::forward_list<std::function<void (Arg &)>> lambdas;

  public:
	auto &get_deltas()    { return deltas;                                        }
	auto &get_lambdas()   { return lambdas;                                       }
	auto num_deltas()     { return deltas.size();                                 }
	auto num_lambdas()    { return std::distance(lambdas.begin(),lambdas.end());  }
	auto num()            { return num_deltas() + num_lambdas();                  }
	auto empty()          { return deltas.empty() && lambdas.empty();             }
	void clear()
	{
		deltas.clear();
		lambdas.clear();
	}

	OpDo &operator()(const typename decltype(lambdas)::value_type &func)
	{
		lambdas.emplace_front(func);
		return *this;
	}

	OpDo &operator()(const Deltas &deltas)
	{
		this->deltas.insert(this->deltas.end(),deltas.begin(),deltas.end());
		return *this;
	}

	OpDo &operator()(const Delta &delta)
	{
		this->deltas.emplace_back(delta);
		return *this;
	}
};
