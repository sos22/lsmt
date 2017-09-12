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
	template <typename stateT, typename visitorT> static void visit(stateT a, visitorT && v) {
		v(a, "key", &key::_key); } };

struct value : meta<value> {
  std::string _value;
	value(std::string const & v) : _value(v) {}
	value(std::string && v) : _value(std::move(v)) {}
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
	int fd = open(path, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd < 0) return false;
	serialiser ser;
	std::string sname(schema.name());
	ser.serialise(sname);
	safe_write(fd, ser.stage, ser.cursor);
	close(fd);
	return true; }

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
	return 0; }
