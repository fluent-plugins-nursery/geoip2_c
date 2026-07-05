#include <ruby.h>
#include <ruby/encoding.h>
#include <maxminddb.h>
#include "rb_compat.h"

VALUE rb_mGeoIP2;
VALUE rb_cGeoIP2Database;
VALUE rb_cGeoIP2LookupResult;
VALUE rb_eGeoIP2Error;

static void mmdb_open(const char *db_path, MMDB_s *mmdb);
static void mmdb_close(MMDB_s *mmdb);
static bool mmdb_is_closed(MMDB_s *mmdb);
static void mmdb_free(void *mmdb);
static MMDB_lookup_result_s mmdb_lookup(MMDB_s *mmdb, const char *ip_str, bool cleanup);
static VALUE mmdb_entry_data_decode(MMDB_entry_data_s *entry_data);
static VALUE mmdb_guard_parse_entry_data_list(VALUE ptr);
static MMDB_entry_data_list_s *mmdb_parse_entry_data_list(MMDB_entry_data_list_s *data_list, bool symbolize_keys, VALUE *obj);

static VALUE rb_geoip2_db_alloc(VALUE klass);
static VALUE rb_geoip2_db_initialize(int argc, VALUE* argv, VALUE self);

static VALUE rb_geoip2_db_close(VALUE self);
static VALUE rb_geoip2_db_lookup(VALUE self, VALUE ip);

static VALUE rb_geoip2_lr_alloc(VALUE klass);
static VALUE rb_geoip2_lr_initialize(VALUE self);
static VALUE rb_geoip2_lr_get_value(int argc, VALUE *argv, VALUE self);
static VALUE rb_geoip2_lr_to_h(VALUE self);

typedef struct {
  struct MMDB_entry_data_list_s* data_list;
  bool symbolize_keys;
} ParseRequest;

/* A LookupResult holds a copy of the libmaxminddb lookup result (whose
 * entry.mmdb points into the owning Database's MMDB_s) together with a
 * reference to that Database VALUE. Keeping the reference and marking it
 * prevents the Database (and its mmap'd file) from being freed while a
 * LookupResult derived from it is still alive. */
typedef struct {
  MMDB_lookup_result_s result;
  VALUE db;
} LookupResult;

static void
lookup_result_mark(void *ptr)
{
  LookupResult *lr = (LookupResult *)ptr;
  rb_gc_mark(lr->db);
}

static const rb_data_type_t rb_mmdb_type = {
  "geoip2/mmdb", {
    0, mmdb_free, 0,
  }, NULL, NULL,
  RUBY_TYPED_FREE_IMMEDIATELY
};

static const rb_data_type_t rb_lookup_result_type = {
  "geoip2/lookup_result", {
    lookup_result_mark, RUBY_DEFAULT_FREE, 0,
  }, NULL, NULL,
  RUBY_TYPED_FREE_IMMEDIATELY
};

/* Fetch the LookupResult and ensure its backing database is still usable.
 * Raises rather than dereferencing a freed/unmapped MMDB_s (use-after-free). */
static LookupResult *
lookup_result_get_open(VALUE self)
{
  LookupResult *lr;
  TypedData_Get_Struct(self, LookupResult, &rb_lookup_result_type, lr);
  if (lr->result.entry.mmdb == NULL ||
      mmdb_is_closed((MMDB_s *)lr->result.entry.mmdb)) {
    rb_raise(rb_eGeoIP2Error, "the database is closed");
  }
  return lr;
}

static void
mmdb_open(const char *db_path, MMDB_s *mmdb)
{
  int status = MMDB_open(db_path, MMDB_MODE_MMAP, mmdb);

  if (status != MMDB_SUCCESS) {
    rb_raise(rb_eGeoIP2Error, "%s: %s", MMDB_strerror(status), db_path);
  }
}

static void
mmdb_close(MMDB_s *mmdb)
{
  MMDB_close(mmdb);
  mmdb->file_content = NULL;
}

static bool
mmdb_is_closed(MMDB_s *mmdb)
{
  return mmdb->file_content == NULL;
}

static void
mmdb_free(void *ptr)
{
  MMDB_s *mmdb = (MMDB_s *)ptr;
  if (!mmdb_is_closed(mmdb)) {
    mmdb_close(mmdb);
  }

  xfree(mmdb);
}

static MMDB_lookup_result_s
mmdb_lookup(MMDB_s *mmdb, const char *ip_str, bool cleanup)
{
  int gai_error;
  int mmdb_error;

  MMDB_lookup_result_s result =
    MMDB_lookup_string(mmdb, ip_str, &gai_error, &mmdb_error);

  if (gai_error != 0) {
    if (cleanup && !mmdb_is_closed(mmdb)) {
      mmdb_close(mmdb);
    }
    rb_raise(rb_eGeoIP2Error,
             "getaddrinfo failed: %s", gai_strerror(gai_error));
  }

  if (mmdb_error != MMDB_SUCCESS) {
    if (cleanup && !mmdb_is_closed(mmdb)) {
      mmdb_close(mmdb);
    }
    rb_raise(rb_eGeoIP2Error,
             "lookup failed: %s", MMDB_strerror(mmdb_error));
  }

  return result;
}

static VALUE
mmdb_entry_data_decode(MMDB_entry_data_s *entry_data)
{
  switch (entry_data->type) {
  case MMDB_DATA_TYPE_EXTENDED:
    /* TODO: not implemented */
    return Qnil;
  case MMDB_DATA_TYPE_POINTER:
    /* TODO: not implemented */
    return Qnil;
  case MMDB_DATA_TYPE_UTF8_STRING:
    return rb_enc_str_new(entry_data->utf8_string,
                          entry_data->data_size,
                          rb_utf8_encoding());
  case MMDB_DATA_TYPE_DOUBLE:
    return DBL2NUM(entry_data->double_value);
  case MMDB_DATA_TYPE_BYTES:
    return rb_enc_str_new((const char *)entry_data->bytes,
                          entry_data->data_size,
                          rb_ascii8bit_encoding());
  case MMDB_DATA_TYPE_UINT16:
    return UINT2NUM(entry_data->uint16);
  case MMDB_DATA_TYPE_UINT32:
    return UINT2NUM(entry_data->uint32);
  case MMDB_DATA_TYPE_MAP:
    /* TODO: not implemented */
    return Qnil;
  case MMDB_DATA_TYPE_INT32:
    return INT2NUM(entry_data->int32);
  case MMDB_DATA_TYPE_UINT64:
    return UINT2NUM(entry_data->uint64);
  case MMDB_DATA_TYPE_UINT128:
    /* uint128 does not fit in a C unsigned long, so build the Integer from the
     * full 128 bits instead of truncating it through UINT2NUM. */
#if !(MMDB_UINT128_IS_BYTE_ARRAY)
    {
      mmdb_uint128_t value = entry_data->uint128;
      return rb_integer_unpack(&value, 1, sizeof(value), 0,
                               INTEGER_PACK_NATIVE_BYTE_ORDER |
                               INTEGER_PACK_LSWORD_FIRST);
    }
#else
    /* entry_data->uint128 is a big-endian uint8_t[16]. */
    return rb_integer_unpack(entry_data->uint128, 16, 1, 0,
                             INTEGER_PACK_BIG_ENDIAN |
                             INTEGER_PACK_MSWORD_FIRST);
#endif
  case MMDB_DATA_TYPE_ARRAY:
    /* TODO: not implemented */
    return Qnil;
  case MMDB_DATA_TYPE_CONTAINER:
    /* TODO: not implemented */
    return Qnil;
  case MMDB_DATA_TYPE_END_MARKER:
    return Qnil;
  case MMDB_DATA_TYPE_BOOLEAN:
    return entry_data->boolean ? Qtrue : Qfalse;
  case MMDB_DATA_TYPE_FLOAT:
    return rb_float_new(entry_data->float_value);
  default:
    rb_raise(rb_eGeoIP2Error, "Unkown type: %d", entry_data->type);
  }
}

static VALUE
mmdb_entry_data_decode_key(MMDB_entry_data_s *entry_data, bool symbolize_strings)
{
  if (entry_data->type != MMDB_DATA_TYPE_UTF8_STRING) {
    rb_raise(rb_eGeoIP2Error, "Unexpected key type: %d", entry_data->type);
  }
  if (symbolize_strings) {
    return ID2SYM(rb_intern3(entry_data->utf8_string,
                             entry_data->data_size,
                             rb_utf8_encoding()));
  } else {
    return rb_enc_str_new(entry_data->utf8_string,
                          entry_data->data_size,
                          rb_utf8_encoding());
  }
}

static VALUE
mmdb_guard_parse_entry_data_list(VALUE ptr)
{
  MMDB_entry_data_list_s *data_list = ((ParseRequest *)ptr)->data_list;
  bool symbolize_keys = ((ParseRequest *)ptr)->symbolize_keys;
  VALUE obj;

  mmdb_parse_entry_data_list(data_list, symbolize_keys, &obj);
  return obj;
}

static MMDB_entry_data_list_s *
mmdb_parse_entry_data_list(MMDB_entry_data_list_s *data_list, bool symbolize_keys, VALUE *obj)
{
  switch (data_list->entry_data.type) {
  case MMDB_DATA_TYPE_MAP:
    {
      uint32_t data_size = data_list->entry_data.data_size;
      VALUE hash = rb_hash_new();
      for (data_list = data_list->next; data_size && data_list; data_size--) {
        VALUE key;
        VALUE val;
        key = mmdb_entry_data_decode_key(&data_list->entry_data, symbolize_keys);
        data_list = data_list->next;
        data_list = mmdb_parse_entry_data_list(data_list, symbolize_keys, &val);
        rb_hash_aset(hash, key, val);
      }
      *obj = hash;
      break;
    }
  case MMDB_DATA_TYPE_ARRAY:
    {
      uint32_t data_size = data_list->entry_data.data_size;
      VALUE array = rb_ary_new();
      for (data_list = data_list->next; data_size && data_list; data_size--) {
        VALUE val;
        data_list = mmdb_parse_entry_data_list(data_list, symbolize_keys, &val);
        rb_ary_push(array, val);
      }
      *obj = array;
      break;
    }
  case MMDB_DATA_TYPE_UTF8_STRING: /* fall through */
  case MMDB_DATA_TYPE_BYTES:
  case MMDB_DATA_TYPE_DOUBLE:
  case MMDB_DATA_TYPE_UINT16:
  case MMDB_DATA_TYPE_UINT32:
  case MMDB_DATA_TYPE_INT32:
  case MMDB_DATA_TYPE_UINT64:
  case MMDB_DATA_TYPE_UINT128:
  case MMDB_DATA_TYPE_BOOLEAN:
  case MMDB_DATA_TYPE_FLOAT:
    {
      VALUE val = mmdb_entry_data_decode(&data_list->entry_data);
      data_list = data_list->next;
      *obj = val;
      break;
    }
  default:
    rb_raise(rb_eGeoIP2Error,
             "Unkown type: %d",
             data_list->entry_data.type);
  }
  return data_list;
}

static VALUE
rb_geoip2_db_alloc(VALUE klass)
{
  VALUE obj;
  MMDB_s *mmdb;
  obj = TypedData_Make_Struct(klass, struct MMDB_s, &rb_mmdb_type, mmdb);
  return obj;
}

static VALUE
rb_geoip2_db_open_mmdb(VALUE self, VALUE path)
{
  char *db_path;
  MMDB_s *mmdb;

  Check_Type(path, T_STRING);

  db_path = StringValueCStr(path);

  TypedData_Get_Struct(self, struct MMDB_s, &rb_mmdb_type, mmdb);

  /* Reopening over an already-open database would overwrite the MMDB_s in
   * place and leak libmaxminddb's internal allocations (and the previous
   * mmap). Close the current database first. */
  if (!mmdb_is_closed(mmdb)) {
    mmdb_close(mmdb);
  }

  mmdb_open(db_path, mmdb);

  return Qnil;
}

static VALUE
rb_geoip2_db_close(VALUE self)
{
  MMDB_s *mmdb;

  TypedData_Get_Struct(self, struct MMDB_s, &rb_mmdb_type, mmdb);

  if (!mmdb_is_closed(mmdb)) {
    mmdb_close(mmdb);
  }

  return Qnil;
}

static VALUE
rb_geoip2_db_lookup(VALUE self, VALUE ip)
{
  char *ip_str;
  MMDB_s *mmdb;
  MMDB_lookup_result_s result;
  LookupResult *result_ptr;
  VALUE obj;

  Check_Type(ip, T_STRING);
  ip_str = StringValueCStr(ip);

  TypedData_Get_Struct(self, struct MMDB_s, &rb_mmdb_type, mmdb);
  result = mmdb_lookup(mmdb, ip_str, false);

  if (!result.found_entry) {
    return Qnil;
  }

  obj = TypedData_Make_Struct(rb_cGeoIP2LookupResult,
                              LookupResult,
                              &rb_lookup_result_type,
                              result_ptr);
  result_ptr->result = result;
  result_ptr->db = self;

  rb_iv_set(obj, "@symbolize_keys", rb_iv_get(self, "@symbolize_keys"));

  return obj;
}

static VALUE
rb_geoip2_lr_alloc(VALUE klass)
{
  VALUE obj;
  LookupResult *result;
  obj = TypedData_Make_Struct(klass,
                              LookupResult,
                              &rb_lookup_result_type,
                              result);
  result->db = Qnil;
  return obj;
}

static VALUE
rb_geoip2_lr_initialize(VALUE self)
{
  return Qnil;
}

static inline char*
rb_geoip2_lr_arg_convert_to_cstring(VALUE sym_or_str)
{
  if (TYPE(sym_or_str) == T_SYMBOL) {
    return RSTRING_PTR(rb_sym2str(sym_or_str));
  } else {
    return StringValueCStr(sym_or_str);
  }
}

static VALUE
rb_geoip2_lr_get_value(int argc, VALUE *argv, VALUE self)
{
  VALUE arg;
  VALUE rest;
  char **path;
  VALUE path_tmp;
  int i = 0;
  LookupResult *result = NULL;
  MMDB_entry_s entry;
  MMDB_entry_data_s entry_data;
  char *tmp;
  VALUE e;
  int status;
  VALUE val;

  rb_scan_args(argc, argv, "1*", &arg, &rest);
  switch(TYPE(arg)) {
    case T_STRING:
    case T_SYMBOL:
      {
        break;
      }
    default:
      rb_raise(rb_eArgError, "Expected a String or a Symbol");
      break;
  }

  result = lookup_result_get_open(self);

  /* ALLOCV_N ties the allocation to path_tmp: it is released automatically even
   * if a later key conversion raises (e.g. a non-String key, or a String with
   * an embedded NUL that makes StringValueCStr raise), and it raises
   * NoMemoryError instead of returning NULL on allocation failure. */
  path = ALLOCV_N(char *, path_tmp, RARRAY_LEN(rest) + 2);

  path[i] = rb_geoip2_lr_arg_convert_to_cstring(arg);
  while (RARRAY_LEN(rest) != 0) {
    ++i;
    e = rb_ary_shift(rest);
    tmp = rb_geoip2_lr_arg_convert_to_cstring(e);
    path[i] = tmp;
  }

  path[i+1] = NULL;

  entry = result->result.entry;

  status = MMDB_aget_value(&entry, &entry_data, (const char *const *const)path);

  if (status != MMDB_SUCCESS) {
    /* fprintf(stderr, "%s:%s\n", __FUNCTION__, MMDB_strerror(status)); */
    ALLOCV_END(path_tmp);
    return Qnil;
  }

  if (!entry_data.has_data) {
    ALLOCV_END(path_tmp);
    return Qnil;
  }

  if (entry_data.type == MMDB_DATA_TYPE_MAP ||
      entry_data.type == MMDB_DATA_TYPE_ARRAY) {
    // FIXME optimize below code
    VALUE array = rb_ary_new();
    VALUE hash;
    bool symbolize_keys = RTEST(rb_iv_get(self, "@symbolize_keys"));
    for (int j = 0; path[j] != NULL; j++) {
      rb_ary_push(array, symbolize_keys ? ID2SYM(rb_intern(path[j])) : rb_str_new_cstr(path[j]));
    }
    hash = rb_funcall(self, rb_intern("to_h"), 0);
    val = rb_apply(hash, rb_intern("dig"), array);
    ALLOCV_END(path_tmp);
    return val;
  }

  val = mmdb_entry_data_decode(&entry_data);
  ALLOCV_END(path_tmp);
  return val;
}

static VALUE
rb_geoip2_lr_netmask(VALUE self)
{
  LookupResult *result = NULL;

  TypedData_Get_Struct(self,
                       LookupResult,
                       &rb_lookup_result_type,
                       result);

  return UINT2NUM(result->result.netmask);
}

static VALUE
rb_geoip2_lr_to_h(VALUE self)
{
  LookupResult *result = NULL;
  MMDB_entry_data_list_s *entry_data_list = NULL;
  VALUE hash;
  VALUE cache;
  int status = 0;
  int exception = 0;

  if (rb_ivar_defined(self, rb_intern("rb_hash")) == Qtrue) {
    cache = rb_ivar_get(self, rb_intern("rb_hash"));
    if (!NIL_P(cache)) {
      return cache;
    }
  }

  result = lookup_result_get_open(self);
  status = MMDB_get_entry_data_list(&result->result.entry, &entry_data_list);
  if (status != MMDB_SUCCESS) {
    rb_raise(rb_eGeoIP2Error, "%s", MMDB_strerror(status));
  }

  ParseRequest parse_request = { entry_data_list, RTEST(rb_iv_get(self, "@symbolize_keys")) };

  hash = rb_protect(mmdb_guard_parse_entry_data_list, (VALUE)&parse_request, &exception);
  MMDB_free_entry_data_list(entry_data_list);

  if (exception != 0) {
    rb_jump_tag(exception);
  }

  rb_ivar_set(self, rb_intern("rb_hash"), hash);

  return hash;
}

void
Init_geoip2(void)
{
  rb_mGeoIP2 = rb_define_module("GeoIP2");
  rb_cGeoIP2Database = rb_define_class_under(rb_mGeoIP2, "Database", rb_cObject);
  rb_cGeoIP2LookupResult = rb_define_class_under(rb_mGeoIP2, "LookupResult", rb_cObject);
  rb_eGeoIP2Error = rb_define_class_under(rb_mGeoIP2, "Error", rb_eStandardError);

  rb_define_alloc_func(rb_cGeoIP2Database, rb_geoip2_db_alloc);
  rb_define_method(rb_cGeoIP2Database, "open_mmdb", rb_geoip2_db_open_mmdb, 1);
  rb_define_method(rb_cGeoIP2Database, "close", rb_geoip2_db_close, 0);
  rb_define_method(rb_cGeoIP2Database, "lookup", rb_geoip2_db_lookup, 1);

  rb_define_alloc_func(rb_cGeoIP2LookupResult, rb_geoip2_lr_alloc);
  rb_define_method(rb_cGeoIP2LookupResult, "initialize", rb_geoip2_lr_initialize, 0);
  rb_define_method(rb_cGeoIP2LookupResult, "get_value", rb_geoip2_lr_get_value, -1);
  rb_define_method(rb_cGeoIP2LookupResult, "netmask", rb_geoip2_lr_netmask, 0);
  rb_define_method(rb_cGeoIP2LookupResult, "to_h", rb_geoip2_lr_to_h, 0);
}
