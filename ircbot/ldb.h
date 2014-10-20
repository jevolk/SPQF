/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


namespace ldb {


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


class const_iterator : public std::iterator<std::bidirectional_iterator_tag,
                                            std::tuple<const char *, size_t, const char *, size_t>>
{
	leveldb::DB *db;
	std::shared_ptr<const leveldb::Snapshot> snap;
	ReadOptions ropt;
	std::unique_ptr<leveldb::Iterator> it;
	mutable value_type kv;

  public:
	// Utils
	virtual bool valid() const                         { return it->Valid();                          }
	int cmp(const const_iterator &o) const;

	operator bool() const                              { return valid();                              }
	auto operator!() const                             { return !valid();                             }

	auto operator==(const const_iterator &o) const     { return cmp(o) == 0;                          }
	auto operator!=(const const_iterator &o) const     { return cmp(o) != 0;                          }
	auto operator<=(const const_iterator &o) const     { return cmp(o) <= 0;                          }
	auto operator>=(const const_iterator &o) const     { return cmp(o) >= 0;                          }
	auto operator<(const const_iterator &o) const      { return cmp(o) < 0;                           }
	auto operator>(const const_iterator &o) const      { return cmp(o) > 0;                           }

	enum Seek { NEXT, PREV, FIRST, LAST, END };
	void seek(const leveldb::Slice &key)               { it->Seek(key);                               }
	void seek(const std::string &key)                  { it->Seek(key);                               }
	void seek(const Seek &seek);

	const_iterator &operator++();
	const_iterator &operator--();
	const_iterator operator++(int);
	const_iterator operator--(int);

	const_iterator &operator+=(const size_t &n);
	const_iterator &operator-=(const size_t &n);
	const_iterator operator+(const size_t &n);
	const_iterator operator-(const size_t &n);

	enum
	{
		KEY, KEY_SIZE,
		VAL, VAL_SIZE,
	};

	value_type &operator*() const;
	value_type *operator->() const;

	const_iterator(leveldb::DB *const &db,
	               const bool &snap        = false,
	               const bool &cache       = false);

	const_iterator(const leveldb::DB *const &db,
	               const bool &snap        = false);

	const_iterator(const const_iterator &other);
	const_iterator &operator=(const const_iterator &other) &;
};


inline
const_iterator::const_iterator(const leveldb::DB *const &db,
                               const bool &snap):
const_iterator(const_cast<leveldb::DB *>(db),snap,false)
{

}


inline
const_iterator::const_iterator(leveldb::DB *const &db,
                               const bool &snap,
                               const bool &cache):
db(db),
snap({snap? db->GetSnapshot() : nullptr, [db](auto&& s) { if(s) db->ReleaseSnapshot(s); }}),
ropt(cache,false,this->snap.get()),
it(db->NewIterator(ropt))
{

}


inline
const_iterator::const_iterator(const const_iterator &o):
db(o.db),
snap(o.snap),
ropt(o.ropt),
it(db->NewIterator(ropt))
{
	if(valid())
		it->Seek(o.it->key());
	else
		seek(END);
}


inline
const_iterator &const_iterator::operator=(const const_iterator &o)
&
{
	db = o.db;
	snap = o.snap;
	ropt = o.ropt;
	it.reset(db->NewIterator(ropt));

	if(valid())
		it->Seek(o.it->key());
	else
		seek(END);

	return *this;
}


inline
const_iterator::value_type *const_iterator::operator->()
const
{
	this->operator*();
	return &kv;
}


inline
const_iterator::value_type &const_iterator::operator*()
const
{
	const leveldb::Slice &key = it->key();
	const leveldb::Slice &val = it->value();
	kv = value_type{key.data(),key.size(),val.data(),val.size()};
	return kv;
}


inline
const_iterator const_iterator::operator+(const size_t &n)
{
	auto ret(*this);
	ret += n;
	return ret;
}


inline
const_iterator const_iterator::operator-(const size_t &n)
{
	auto ret(*this);
	ret -= n;
	return ret;
}


inline
const_iterator &const_iterator::operator+=(const size_t &n)
{
	for(size_t i = 0; i < n; i++)
		seek(NEXT);

	return *this;
}


inline
const_iterator &const_iterator::operator-=(const size_t &n)
{
	for(size_t i = 0; i < n; i++)
		seek(PREV);

	return *this;
}


inline
const_iterator const_iterator::operator++(int)
{
	auto ret(*this);
	++(*this);
	return ret;
}


inline
const_iterator const_iterator::operator--(int)
{
	auto ret(*this);
	--(*this);
	return ret;
}


inline
const_iterator &const_iterator::operator++()
{
	seek(NEXT);
	return *this;
}


inline
const_iterator &const_iterator::operator--()
{
	seek(PREV);
	return *this;
}


inline
void const_iterator::seek(const Seek &seek)
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
int const_iterator::cmp(const const_iterator &o)
const
{
	return !valid() && !o.valid()?  0:
	       !valid() && o.valid()?   1:
	       valid() && !o.valid()?  -1:
	                                it->key().compare(o.it->key());
}


class Ldb
{
	std::unique_ptr<leveldb::Cache> cache;
	std::unique_ptr<const leveldb::FilterPolicy> fp;
	Opts opts;
	std::unique_ptr<leveldb::DB> db;

  public:
	using const_iterator = ldb::const_iterator;

	// Reading
	template<class... Args> const_iterator end(Args&&... args) const;
	template<class... Args> const_iterator begin(Args&&... args) const;
	template<class... Args> const_iterator find(const std::string &key, Args&&... args) const;

	// Utils
	size_t count() const;
	bool exists(const std::string &key) const;
	bool exists(const std::string &key, const bool &cache = false);

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
bool Ldb::exists(const std::string &key,
                 const bool &cache)
{
	const auto it = find(key,false,cache);
	return bool(it);
}


inline
bool Ldb::exists(const std::string &key)
const
{
	const auto it = find(key,false,false);
	return bool(it);
}


inline
size_t Ldb::count()
const
{
	size_t ret = 0;
	const auto end = this->end();
	for(auto it = begin(); it != end; ++it)
		ret++;

	return ret;
}


template<class... Args>
Ldb::const_iterator Ldb::end(Args&&... args)
const
{
	const_iterator it(db.get(),std::forward<Args>(args)...);
	it.seek(const_iterator::END);
	return it;
}


template<class... Args>
Ldb::const_iterator Ldb::begin(Args&&... args)
const
{
	const_iterator it(db.get(),std::forward<Args>(args)...);
	it.seek(const_iterator::FIRST);
	return it;
}


template<class... Args>
Ldb::const_iterator Ldb::find(const std::string &key,
                              Args&&... args)
const
{
	const_iterator it(db.get(),std::forward<Args>(args)...);
	it.seek(key);

	if(!it)
		it.seek(const_iterator::END);

	return it;
}


inline
ldb::ReadOptions::ReadOptions(const bool &cache,
                              const bool &verify,
                              const leveldb::Snapshot *const &snap)
{
	this->verify_checksums  = verify;
	this->fill_cache        = fill_cache;
	this->snapshot          = snapshot;
}



inline
ldb::WriteOptions::WriteOptions(const bool &sync)
{
	this->sync = sync;
}



inline
ldb::Opts::Opts(leveldb::Cache *const cache,
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


} // namespace ldb
