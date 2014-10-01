/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Ldb
{
	struct Opts : public leveldb::Options
	{
		Opts(leveldb::Cache *const cache         = nullptr,
		     const leveldb::FilterPolicy *fp     = nullptr,
		     const leveldb::CompressionType ct   = leveldb::kSnappyCompression,
		     const size_t block_size             = 16384,
		     const bool create_if_missing        = true,
		     const size_t write_buffer_size      = 4 * (1024 * 1024),
		     const size_t max_open_files         = 1024);
	};

	std::unique_ptr<leveldb::Cache> cache;
	std::unique_ptr<const leveldb::FilterPolicy> fp;
	Opts opts;
	std::unique_ptr<leveldb::DB> db;

  public:
	struct WriteOptions : public leveldb::WriteOptions
	{
		WriteOptions(const bool &sync = false);
	};

	struct ReadOptions : public leveldb::ReadOptions
	{
		ReadOptions(const bool &cache                      = false,
		            const bool &verify                     = false,
		            const leveldb::Snapshot *const &snap   = nullptr);
	};

	class Iterator
	{
		leveldb::DB &db;
		const leveldb::Snapshot *snap;
		const ReadOptions ropt;
		std::unique_ptr<leveldb::Iterator> it;

	  public:
		enum Seek { NEXT, PREV, FIRST, LAST, END };
		using Closure = std::function<void (const char *const &key, const size_t &,        // key
		                                    const char *const &val, const size_t &)>;      // val

		using StrClosure = std::function<void (const std::string &key, const std::string &val)>;

		// Utils
		bool valid() const                                    { return it->Valid();                 }

		// Move the state pointer
		void seek(const leveldb::Slice &key)                  { it->Seek(key);                      }
		void seek(const std::string &key)                     { it->Seek(key);                      }
		void seek(const Seek &seek);
		Iterator &operator++()                                { seek(NEXT); return *this;           }
		Iterator &operator--()                                { seek(PREV); return *this;           }

		// Read current state, false on !valid()
		bool next(const Closure &closure) const;              // no increment to next
		bool next(const Closure &closure);                    // increments to next
		bool next(const StrClosure &closure) const;           // no increment to next
		bool next(const StrClosure &closure);                 // increments to next
		template<class... A> bool operator()(A&&... a) const  { return next(std::forward<A>(a)...); }
		template<class... A> bool operator()(A&&... a)        { return next(std::forward<A>(a)...); }

		// Dependent utils
		size_t count();

		Iterator(Ldb &ldb,
		         const bool &first       = true,
		         const bool &cache       = false,
		         const bool &snap        = true);

		Iterator(Ldb &ldb,
		         const std::string &key,
		         const bool &exact_match = true,              // false = seek to next closest key
		         const bool &cache       = false,
		         const bool &snap        = true);

		~Iterator() noexcept;
	};

	using Closure = Iterator::Closure;

	// Utils
	size_t count() const;
	bool exists(const std::string &key) const;
	bool exists(const std::string &key, const bool &cache = false);

	// Reading
	bool get(const std::nothrow_t, const std::string &key, const Closure &closure, const bool &cache = true) noexcept;
	bool get(const std::nothrow_t, const std::string &key, const Closure &closure) const noexcept;
	void get(const std::string &key, const Closure &closure, const bool &cache = true);
	void get(const std::string &key, const Closure &closure) const;

	std::string get(const std::nothrow_t, const std::string &key, const bool &cache = true) noexcept;
	std::string get(const std::nothrow_t, const std::string &key) const noexcept;
	std::string get(const std::string &key, const bool &cache = true);
	std::string get(const std::string &key) const;

	// Writing
	void set(const std::string &key, const std::string &value, const bool &sync = false);

	Ldb(const std::string &dir,
	    const size_t &cache_size  = (32UL * 1024UL * 1024UL),
	    const size_t &bloom_bits  = 0);
};


inline
Ldb::Ldb(const std::string &dir,
         const size_t &cache_size,
         const size_t &bloom_bits):
cache(cache_size? leveldb::NewLRUCache(cache_size) : nullptr),
fp(bloom_bits? leveldb::NewBloomFilterPolicy(bloom_bits) : nullptr),
opts(cache.get(),fp.get(),leveldb::kSnappyCompression),
db([&]() -> leveldb::DB *
{
	leveldb::DB *ret;
	const leveldb::Status s = leveldb::DB::Open(opts,dir,&ret);
	std::cout << "Leveldb::DB::Open(" << dir << "): " << s.ToString() << std::endl;

	if(!s.ok())
		throw Exception("Failed to start levelDB");

	return ret;
}())
{


}


inline
void Ldb::set(const std::string &key,
              const std::string &value,
              const bool &sync)
{
	const WriteOptions wopt(sync);
	const leveldb::Status stat = db->Put(wopt,key,value);
	if(!stat.ok())
		throw Exception(stat.ToString());
}


inline
std::string Ldb::get(const std::string &key)
const
{
	const std::string ret = get(std::nothrow,key);
	return ret;
}


inline
std::string Ldb::get(const std::string &key,
                     const bool &cache)
{
	const std::string ret = get(std::nothrow,key,cache);
	if(ret.empty())
		throw Exception("Key not found in database");

	return ret;
}


inline
std::string Ldb::get(const std::nothrow_t,
                     const std::string &key)
const noexcept
{
	std::string ret;
	const auto closure = [&ret]
	(const char *const &key, const size_t &ks, const char *const &val, const size_t &vs)
	{
		ret = {val,vs};
	};

	get(std::nothrow,key,closure);
	return ret;
}


inline
std::string Ldb::get(const std::nothrow_t,
                     const std::string &key,
                     const bool &cache)
noexcept
{
	std::string ret;
	const auto closure = [&ret]
	(const char *const &key, const size_t &ks, const char *const &val, const size_t &vs)
	{
		ret = {val,vs};
	};

	get(std::nothrow,key,closure,cache);
	return ret;
}


inline
void Ldb::get(const std::string &key,
              const Closure &closure)
const
{
	if(!get(std::nothrow,key,closure))
		throw Exception("Key not found in database");
}


inline
void Ldb::get(const std::string &key,
              const Closure &closure,
              const bool &cache)
{
	if(!get(std::nothrow,key,closure,cache))
		throw Exception("Key not found in database");
}


inline
bool Ldb::get(const std::nothrow_t,
              const std::string &key,
              const Closure &closure)
const noexcept
{
	const Iterator it(const_cast<Ldb &>(*this),key,true,false,true);
	return it.next(closure);
}


inline
bool Ldb::get(const std::nothrow_t,
              const std::string &key,
              const Closure &closure,
              const bool &cache)
noexcept
{
	const Iterator it(*this,key,true,cache,true);
	return it.next(closure);
}


inline
bool Ldb::exists(const std::string &key,
                 const bool &cache)
{
	const Iterator it(*this,key,true,cache,false);
	return it.valid();
}


inline
bool Ldb::exists(const std::string &key)
const
{
	const Iterator it(const_cast<Ldb &>(*this),key,true,false,false);
	return it.valid();
}


inline
size_t Ldb::count()
const
{
	Iterator it(const_cast<Ldb &>(*this),true,false,true);
	return it.count();
}



inline
Ldb::Iterator::Iterator(Ldb &ldb,
                        const std::string &key,
                        const bool &exact_match,
                        const bool &cache,
                        const bool &snap):
Iterator(ldb,false,cache,snap)
{
	seek(key);

	if(exact_match && valid() && it->key() != key)
		seek(END);
}


inline
Ldb::Iterator::Iterator(Ldb &ldb,
                        const bool &first,
                        const bool &cache,
                        const bool &snap):
db(*ldb.db),
snap(snap? db.GetSnapshot() : nullptr),
ropt(cache,false,this->snap),
it(db.NewIterator(ropt))
{
	if(first)
		seek(FIRST);
}


inline
Ldb::Iterator::~Iterator()
noexcept
{
	if(snap)
		db.ReleaseSnapshot(snap);
}


inline
size_t Ldb::Iterator::count()
{
	const leveldb::Slice cur_key = it->key();

	size_t ret;
	for(ret = 0; valid(); ret++)
		seek(NEXT);

	seek(cur_key);
	return ret;
}


inline
bool Ldb::Iterator::next(const StrClosure &closure)
{
	return next([&closure]
	(const char *const &key, const size_t &ks, const char *const &val, const size_t &vs)
	{
		closure({key,ks},{val,vs});
	});
}


inline
bool Ldb::Iterator::next(const StrClosure &closure) const
{
	return next([&closure]
	(const char *const &key, const size_t &ks, const char *const &val, const size_t &vs)
	{
		closure({key,ks},{val,vs});
	});
}


inline
bool Ldb::Iterator::next(const Closure &closure)
{
	if(!valid())
		return false;

	const leveldb::Slice &key = it->key();
	const leveldb::Slice &value = it->value();
	closure(key.data(),key.size(),value.data(),value.size());
	const scope nxt([&]{ seek(NEXT); });
	return true;
}


inline
bool Ldb::Iterator::next(const Closure &closure)
const
{
	if(!valid())
		return false;

	const leveldb::Slice &key = it->key();
	const leveldb::Slice &value = it->value();
	closure(key.data(),key.size(),value.data(),value.size());
	return true;
}


inline
void Ldb::Iterator::seek(const Seek &seek)
{
	switch(seek)
	{
		case NEXT:     it->Next();                      break;
		case PREV:     it->Prev();                      break;
		case FIRST:    it->SeekToFirst();               break;
		case LAST:     it->SeekToLast();                break;
		case END:      it->SeekToLast();  it->Next();   break;
	};
}


inline
Ldb::ReadOptions::ReadOptions(const bool &cache,
                              const bool &verify,
                              const leveldb::Snapshot *const &snap)
{
	this->verify_checksums  = verify;
	this->fill_cache        = fill_cache;
	this->snapshot          = snapshot;
}



inline
Ldb::WriteOptions::WriteOptions(const bool &sync)
{
	this->sync = sync;
}



inline
Ldb::Opts::Opts(leveldb::Cache *const cache,
                const leveldb::FilterPolicy *const fp,
                const leveldb::CompressionType ct,
                const size_t block_size,
                const bool create_if_missing,
                const size_t write_buffer_size,
                const size_t max_open_files)
{
//	this->comparator         = nullptr;
	this->filter_policy      = fp;
	this->compression        = ct;
	this->block_cache        = cache;
	this->block_size         = block_size;
	this->write_buffer_size  = write_buffer_size;
	this->max_open_files     = max_open_files;
	this->create_if_missing  = create_if_missing;
}

