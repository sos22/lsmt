#include "lsmt.H"

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>
#include <err.h>
#include <unistd.h>

#include <map>
#include <typeinfo>

#include "meta.H"
#include "serialise.H"

template <typename keyT, typename valueT> struct schema {
	using key = keyT;
	using value = valueT; };

struct key : meta<key> {
	std::string _key;
	key(std::string const & k) : _key(k) {}
	key(std::string && k) : _key(std::move(k)) {}
	key() : _key("") {}
	key(char const * k) : _key(k) {}
	template <typename stateT, typename visitorT> static void visit(stateT a, visitorT && v) {
		v(a, "key", &key::_key); } };

struct value : meta<value> {
  std::string _value;
	value(std::string const & v) : _value(v) {}
	value(std::string && v) : _value(std::move(v)) {}
	value(char const * k) : _value(k) {}
	value() : _value("") {}
  template <typename stateT, typename visitorT> static void visit(stateT a, visitorT && v) {
		v(a, "value", &value::_value); } };

struct untyped_ingress {
	int fd{-1};
	serialiser ser;
  bool open(char const * path);
	~untyped_ingress();
	bool flush(); }; 

bool untyped_ingress::open(char const * path) {
	int fd_ = ::open(path, O_WRONLY|O_APPEND);
	if (fd_ < 0) return false;
	if (fd >= 0 && close(fd) < 0) err(1, "reclose ingress");
	fd = fd_;
	return true; }

untyped_ingress::~untyped_ingress() {
	if (fd < 0) return;
	if (!flush()) err(1, "final flush");
	if (close(fd) < 0) err(1, "clos destroy untyped ingress"); }
	
void safe_write(int fd, void const * stage, size_t sz) {
	size_t cursor = 0;
	ssize_t delta;
	while (cursor < sz) {
		delta = write(fd,
									reinterpret_cast<void const *>(reinterpret_cast<uint64_t>(stage) + cursor),
									sz - cursor);
		if (delta < 0) err(1, "write");
		cursor += delta; } }

bool safe_read(int fd, void * stage, size_t sz) {
	size_t cursor = 0;
	ssize_t delta;
	while (cursor < sz) {
		delta = read(fd,
								 reinterpret_cast<void *>(reinterpret_cast<uint64_t>(stage) + cursor),
								 sz - cursor);
		if (delta <= 0) return false;
		cursor += sz; }
	return true; }

void safe_pread(int fd, void * stage, size_t sz, size_t offset) {
	size_t cursor = offset;
	while (cursor < offset + sz) {
		auto delta = ::pread(fd,
												 reinterpret_cast<void *>(reinterpret_cast<size_t>(stage) + cursor - offset),
												 sz - (cursor - offset),
												 cursor);
		assert(delta > 0);
		cursor += delta; } }

bool untyped_ingress::flush() {
	if (fd < 0) errx(1, "flush on un-opened ingress?");
	safe_write(fd, ser.stage, ser.cursor);
	ser.cursor = 0;
	return true; }

struct ingress_restore {
	void * map{nullptr};
	size_t sz{0}; // map size, not deserialise size.
	deserialiser des;
	bool open(char const * path, std::type_info const & schema);
	~ingress_restore(); };

bool ingress_restore::open(char const * path, std::type_info const & schema) {
	int fd = ::open(path, O_RDONLY);
	if (fd < 0) return false;
	struct stat st;
	if (fstat(fd, &st) < 0) {
		close(fd);
		return false; }
	size_t size = (st.st_size + 4095) & ~4095;
	void * _map = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if (_map == MAP_FAILED) {
		close(fd);
		return false; }
	des.open(_map, st.st_size);
	close(fd);
	std::string schema_name;
	des.deserialise(schema_name);
	if (des.failed()) errx(1, "failed to read header");
	if (schema_name != schema.name()) {
		errx(1, "%s was supposed to be %s, got %s",
				 path,
				 schema.name(),
				 schema_name.c_str()); }
	// Openned.
	map = _map;
	sz = size; 
	return true; }

ingress_restore::~ingress_restore() {
	des.close();
	munmap(map, sz); }

struct ingress_create {
	bool create(char const * path, std::type_info const & schema); };
	
template <typename schema>
struct ingress {
	std::map<typename schema::key, typename schema::value> cache;
	untyped_ingress ut;

	bool create(char const * path) {
		ingress_create ic;
		return ic.create(path, typeid(schema)) &&
			ut.open(path); }

	bool open(char const * path) {
		ingress_restore ir;
		if (!ir.open(path, typeid(schema))) return false;
		while (!ir.des.finished()) {
			typename schema::key k;
			typename schema::value v;
			ir.des.deserialise(k);
			ir.des.deserialise(v);
			if (ir.des.failed()) return false;
			cache[k] = v; }
		return ut.open(path); }

  void accept(typename schema::key const & k, typename schema::value const & v) {
		ut.ser.serialise(k);
		ut.ser.serialise(v);
		cache[k] = v;
		ut.flush(); }

	typename schema::value lookup(typename schema::key const & k) { return cache.at(k); } };

bool ingress_create::create(char const * path, std::type_info const & schema) {
	int fd = open(path, O_WRONLY|O_CREAT|O_EXCL, 0643);
	if (fd < 0) return false;
	serialiser ser;
	std::string sname(schema.name());
	ser.serialise(sname);
	safe_write(fd, ser.stage, ser.cursor);
	close(fd);
	return true; }

// A thing for writing one level in the lsmt, when we need to push out the ingress structure.
//
// Serialised format:
// <schema identifier>
// <values>
// <list of key offsets, relative to start of key section>
// <pairs of pointers to values, relative to start of value section, and keys>
// <pointer to key offsets, relative to start of value section>
// <number of pairs>
//
// For now, I'm going to assume it's small enough to hold all keys in memory while we're working,
// and that we don't need a proper index. We can maybe fix both of those at the same time:
// build blocks of values and keys, which can be flushed out whenever, keeping the first
// key in the block to form an entry in the index, which flushes when index blocks get full.
//
// Note: value pointers come before serialised key in key index section, meaning that
// if we've found and deserialised a key we can trivially find its value pointer and the
// next value pointer, giving us the size of the referenced value.
struct untyped_stratum_writer {
	serialiser value_serialiser;
	serialiser key_offset_serialiser;
	serialiser key_serialiser;
	unsigned nr_pairs{0};
	bool write(char const * path, std::type_info const & schema); };
		
template <typename schema>
struct stratum_writer {
	untyped_stratum_writer ut;
	void accept(typename schema::key const & k, typename schema::value const & v) {
		auto o(ut.value_serialiser.cursor);
		ut.value_serialiser.serialise(v);
		ut.key_offset_serialiser.serialise(ut.key_serialiser.cursor);
		ut.key_serialiser.serialise(o);
		ut.key_serialiser.serialise(k);
		ut.nr_pairs++; }
	bool finalise(char const * path, ingress<schema> const & inp) {
		for (auto it : inp.cache) accept(it.first, it.second);
		return ut.write(path, typeid(schema)); } };

bool untyped_stratum_writer::write(char const * filename, std::type_info const & schema) {
	int fd = open(filename, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd < 0) return false;
	auto write_serialiser = [fd] (serialiser const & w) { safe_write(fd, w.stage, w.cursor); };
	{ std::string ident{schema.name()};
		serialiser ser;
		ser.serialise(ident);
		write_serialiser(ser); }
	write_serialiser(value_serialiser);
	write_serialiser(key_offset_serialiser);
	write_serialiser(key_serialiser);
	serialiser ser;
	ser.serialise(value_serialiser.cursor);
	ser.serialise(nr_pairs);
	write_serialiser(ser);
	close(fd); 
	return true; }

struct slice {
	// Slice of a file. Inclusive on start, exclusive on end.
	uint64_t start{0};
	uint64_t end{0}; };

// Heavy on system calls and no caching. Deliberately, so that I can test them in moderate
// isolation. Will definitely need caching later.
struct untyped_stratum_reader {
	int fd{-1};
	unsigned nr_pairs{0};
	// Offsets of sections in file.
	uint64_t values{0};
	uint64_t key_offsets{0};
	uint64_t keys{0};
	uint64_t end_of_keys{0};
	bool open(char const * fname, std::type_info const & schema);
	// slice covering the pointer to value and the key, in the key section
	slice locate_key(unsigned idx) const;
	slice read_key(slice, void * out) const;
	void read_value(slice where, void * out) const;
};
	
// Ewww...
bool untyped_stratum_reader::open(char const * fname, std::type_info const & schema) {
	if (fd >= 0) close(fd);
	fd = ::open(fname, O_RDONLY);
	if (fd < 0) return false;
	// Check schema name.
	{ serialiser ser;
		std::string desired_name(schema.name());
		ser.serialise(desired_name);
		char buf[ser.cursor];
		if (!safe_read(fd, buf, sizeof(buf))) {
			close(fd);
			return false; }
		std::string actual_name;
		deserialiser des(buf, sizeof(buf));
		des.deserialise(actual_name);
		if (des.failed() || !des.finished() || desired_name != actual_name) {
			close(fd);
			return false; }
		values = ser.cursor; }
	// How big is the file in total?
	uint64_t total_size;
	{ struct stat st;
		if (::fstat(fd, &st) < 0) {
			close(fd);
			return false; }
		total_size = st.st_size; }
	// Parse the last couple of offsets.
	{ serialiser ser;
		ser.serialise(key_offsets);
		ser.serialise(nr_pairs);
		char buf[ser.cursor];
		safe_pread(fd, buf, sizeof(buf), total_size - sizeof(buf));
		deserialiser des(buf, sizeof(buf));
		des.deserialise(key_offsets);
		des.deserialise(nr_pairs);
		if (des.failed() || !des.finished()) {
			close(fd);
			return false; } }
	key_offsets += values;
	keys = key_offsets + 8 * nr_pairs;
	// Simple sanity check, while we're here. Every pair needs at least 8 bytes in the keys
	// area, for the value pointer, and we need 12 bytes for the trailer.
	if (total_size < keys + 8 * nr_pairs + 8 + 4) {
		close(fd);
		return false; } 
	end_of_keys = total_size - 12;
	return true; }
		
slice untyped_stratum_reader::locate_key(unsigned idx) const {
	uint64_t buf[2];
	safe_pread(fd, buf, sizeof(buf), key_offsets + idx * 8);
	slice res;
	res.start = buf[0] + keys;
	res.end = buf[1] + keys;
	if (idx == nr_pairs - 1) {
		// Last key is special.
		res.end = end_of_keys; }
	return res; } 
	
template <typename schema>
struct stratum_reader {
	untyped_stratum_reader ut;
	bool open(char const * fname) { return ut.open(fname, typeid(schema)); }
	typename schema::value fetch(typename schema::key const & k) const {
		unsigned left{0}; // Target is at or to the right of this one.
		unsigned right{ut.nr_pairs}; // Target is definitely left of this one.
		unsigned desired_idx;
		slice value_where;
		while (true) {
			unsigned probe_idx = (left + right) / 2;
			key probe_key = read_one_key(probe_idx, &value_where);
			auto ord = k.compare(probe_key);
			if (ord == order::eq) {
				desired_idx = probe_idx;
				break; }
			else if (ord == order::gt) left = probe_idx + 1;
			else {
				assert(ord == order::lt);
				right = probe_idx; }
			assert(right != left); // or else key not present.
		}
		return read_one_value(value_where); }
	typename schema::key read_one_key(unsigned idx, slice * value_pos) const {
		auto key_slice = ut.locate_key(idx);
		if (idx < ut.nr_pairs) {
			// Extend to cover the next value pointer.
			key_slice.end += 8; }
		else {
			// Otherwise, next value is implicitly the end of the value section.
		}
		char buf[key_slice.end - key_slice.start];
		safe_pread(ut.fd, buf, sizeof(buf), key_slice.start);
		deserialiser deser(buf, sizeof(buf));
		deser.deserialise(value_pos->start);
		value_pos->start += ut.values;
		typename schema::key result;
		deser.deserialise(result);
		if (idx < ut.nr_pairs) {
			deser.deserialise(value_pos->end);
			value_pos->end += ut.values; }
		else value_pos->end = ut.key_offsets;
		// TODO assert really isn't the right answer here.
		assert(!deser.failed());
		assert(deser.finished());
		return result; }
	typename schema::value read_one_value(const slice & where) const {
		char buf[where.end - where.start];
		safe_pread(ut.fd, buf, sizeof(buf), where.start);
		deserialiser des(buf, sizeof(buf));
		typename schema::value res;
		des.deserialise(res);
		assert(des.finished());
		assert(!des.failed());
		return res; } };

using test_schema = schema<key, value>;
int main() {
	char const * const fname = "testfile";
	{ int r = unlink(fname);
		assert(r == 0 || errno == ENOENT); }
	{ ingress<test_schema> it;
		bool b = it.create(fname);
		assert(b);	
		it.accept(key("key1"), value("value1"));
		assert(it.lookup(key("key1")) == value("value1")); }
	{ ingress<test_schema> it;
		bool b = it.open(fname);
		assert(b);
		assert(it.lookup(key("key1")) == value("value1"));
		it.accept(key("key2"), value("value2"));
		assert(it.lookup(key("key2")) == value("value2"));
		assert(it.lookup(key("key1")) == value("value1")); }
	{ ingress<test_schema> it;
		bool b = it.open(fname);
		assert(b);
		assert(it.lookup(key("key2")) == value("value2"));
		assert(it.lookup(key("key1")) == value("value1"));
		it.accept(key("key1"), value("value3"));
		assert(it.lookup(key("key1")) == value("value3")); }
	{ ingress<test_schema> it;
		bool b = it.open(fname);
		assert(b);
		assert(it.lookup(key("key1")) == value("value3"));
		assert(it.lookup(key("key2")) == value("value2")); }
	{ ingress<test_schema> it;
		bool b = it.open(fname);
		assert(b);
		stratum_writer<test_schema> writer;
		writer.finalise("testfile2", it); }
	{ stratum_reader<test_schema> reader;
		bool b = reader.open("testfile2");
		assert(b);
		printf("nr pairs %u\n", reader.ut.nr_pairs);
		printf("values %lx, key_offsets %lx, end_of_keys %lx\n",
			reader.ut.values, reader.ut.key_offsets, reader.ut.end_of_keys);
		for (unsigned x = 0; x < reader.ut.nr_pairs; x++) {
			slice value_pos;
			key k = reader.read_one_key(x, & value_pos);
			printf("Key %d -> %s; %ld, %ld\n", x, k._key.c_str(), value_pos.start, value_pos.end);
			value v = reader.read_one_value(value_pos);
			printf("value %d -> %s\n", x, v._value.c_str()); }
		assert(reader.fetch(key("key1")) == "value3");
		assert(reader.fetch("key2") == "value2"); }
	return 0; }
