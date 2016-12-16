#include <ruby.h>
#include <maxminddb.h>

VALUE rb_mGeoIP2;
VALUE rb_cGeoIP2Database;
VALUE rb_cGeoIP2SearchResult;
VALUE rb_eGeoIP2Error;

static void mmdb_open(const char *db_path, MMDB_s *mmdb);
static void mmdb_close(MMDB_s *mmdb);
static bool mmdb_is_closed(MMDB_s *mmdb);
static MMDB_lookup_result_s mmdb_lookup(MMDB_s *mmdb, const char *ip_str, bool cleanup);

static VALUE rb_geoip2_db_alloc(VALUE self);
static void rb_geoip2_db_free(MMDB_s *mmdb);
static VALUE rb_geoip2_db_initialize(VALUE self, VALUE path);
static VALUE rb_geoip2_db_close(VALUE self);
static VALUE rb_geoip2_db_lookup(VALUE self, VALUE ip);

// static void lookup_result_free(void *pointer);
// static size_t lookup_result_size(void *pointer);

static const rb_data_type_t lookup_result_type = {
  "geoip2/lookup_result",
  {0, 0, 0,},
};

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
  /*
  if (!result.found_entry) {
    return Qnil;
  }

  MMDB_entry_data_list_s *entry_data_list = NULL;
  int status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);

  if (status != MMDB_SUCCESS) {
    MMDB_free_entry_data_list(entry_data_list);
    if (cleanup && !mmdb_is_closed(mmdb)) {
      mmdb_close(mmdb);
    }
    rb_raise(rb_eGeoIP2Error,
             "failed to fetch result: %s", MMDB_strerror(status));
  }

  return TypedData_Wrap_Struct(rb_cGeoIP2SearchResult,
                               &lookup_result_type,
                               entry_data_list);
  */
}


static VALUE
rb_geoip2_db_alloc(VALUE klass)
{
  MMDB_s *mmdb = RB_ALLOC(MMDB_s);
  return Data_Wrap_Struct(klass, NULL, rb_geoip2_db_free, mmdb);
}

static void
rb_geoip2_db_free(MMDB_s *mmdb)
{
  if (!mmdb_is_closed(mmdb)) {
    mmdb_close(mmdb);
  }

  xfree(mmdb);
}

static VALUE
rb_geoip2_db_initialize(VALUE self, VALUE path)
{
  char *db_path;
  MMDB_s *mmdb;

  Check_Type(path, T_STRING);

  db_path = StringValueCStr(path);

  Data_Get_Struct(self, MMDB_s, mmdb);
  mmdb_open(db_path, mmdb);

  return Qnil;
}

static VALUE
rb_geoip2_db_close(VALUE self)
{
  MMDB_s *mmdb;

  Data_Get_Struct(self, MMDB_s, mmdb);

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

  Check_Type(ip, T_STRING);
  ip_str = StringValueCStr(ip);

  Data_Get_Struct(self, MMDB_s, mmdb);
  result = mmdb_lookup(mmdb, ip_str, false);

  return TypedData_Wrap_Struct(rb_cGeoIP2SearchResult,
                               &lookup_result_type,
                               &result);
}

static VALUE
rb_geoip2_sr_dig(int argc, VALUE *argv, VALUE self)
{
  VALUE arg;
  VALUE rest;
  char **path;
  int i = 0;
  MMDB_lookup_result_s *result = NULL;
  MMDB_entry_s entry;
  MMDB_entry_data_s entry_data;
  char *tmp;
  VALUE e;

  rb_scan_args(argc, argv, "1*", &arg, &rest);
  Check_Type(arg, T_STRING);
  path = malloc(sizeof(char *) * RARRAY_LEN(rest));

  TypedData_Get_Struct(self, MMDB_lookup_result_s, &lookup_result_type, result);

  path[i] = StringValueCStr(arg);
  while (RARRAY_LEN(rest) != 0) {
    ++i;
    e = rb_ary_shift(rest);
    tmp = StringValueCStr(e);
    path[i] = tmp;
  }

  entry = result->entry;

  MMDB_aget_value(&entry, &entry_data, (const char *const *const)path);
  free(path);

  if (!entry_data.has_data) {
    return Qnil;
  }

  switch (entry_data.type) {
  case MMDB_DATA_TYPE_UTF8_STRING:
    return rb_str_new(entry_data.utf8_string, entry_data.data_size);
    break;
  default:
    rb_raise(rb_eGeoIP2Error, "Unkown type: %d", entry_data.type);
  }

  return Qnil;
}


void
Init_geoip2(void)
{
  rb_mGeoIP2 = rb_define_module("GeoIP2");
  rb_cGeoIP2Database = rb_define_class_under(rb_mGeoIP2, "Database", rb_cObject);
  rb_cGeoIP2SearchResult = rb_define_class_under(rb_mGeoIP2, "SearchResult", rb_cData);
  rb_eGeoIP2Error = rb_define_class_under(rb_mGeoIP2, "Error", rb_eStandardError);

  rb_define_method(rb_cGeoIP2Database, "initialize", rb_geoip2_db_initialize, 1);
  rb_define_method(rb_cGeoIP2Database, "allocate", rb_geoip2_db_alloc, 0);
  rb_define_method(rb_cGeoIP2Database, "close", rb_geoip2_db_close, 0);
  rb_define_method(rb_cGeoIP2Database, "lookup", rb_geoip2_db_lookup, 1);

  rb_define_method(rb_cGeoIP2SearchResult, "dig", rb_geoip2_sr_dig, -1);
}
