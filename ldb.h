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
		enum Seek { FIRST, LAST, NEXT, PREV };
		using Closure = std::function<void (const char *const &key, const size_t &,        // key
		                                    const char *const &val, const size_t &)>;      // val

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
		         const bool &cache       = false,
		         const bool &snap        = true);

		~Iterator() noexcept;
	};

	using Closure = Iterator::Closure;

	// Utils
	size_t count();
	bool exists(const std::string &key, const bool &cache = false);

	// Reading
	bool get(const std::string &key, const Closure &closure, const bool &cache = true);
	std::string get(const std::string &key, const bool &cache = true);

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
opts(cache.get(),
     fp.get(),
     leveldb::kSnappyCompression),
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
std::string Ldb::get(const std::string &key,
                     const bool &cache)
{
	std::string ret;
	const auto closure = [&ret]
	(const char *const &key, const size_t &ks, const char *const &val, const size_t &vs)
	{
		ret = {val,vs};
	};

	get(key,closure,cache);
	return ret;
}


inline
bool Ldb::get(const std::string &key,
              const Closure &closure,
              const bool &cache)
{
	const Iterator it(*this,key,cache,true);
	return it.next(closure);
}


inline
bool Ldb::exists(const std::string &key,
                 const bool &cache)
{
	const Iterator it(*this,key,cache,false);
	return it.valid();
}


inline
size_t Ldb::count()
{
	Iterator it(*this,true,false,true);
	return it.count();
}



inline
Ldb::Iterator::Iterator(Ldb &ldb,
                        const std::string &key,
                        const bool &cache,
                        const bool &snap):
Iterator(ldb,false,cache,snap)
{
	seek(key);
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
bool Ldb::Iterator::next(const Closure &closure) const
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
		case FIRST:    it->SeekToFirst();       break;
		case LAST:     it->SeekToLast();        break;
		case NEXT:     it->Next();              break;
		case PREV:     it->Prev();              break;
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

