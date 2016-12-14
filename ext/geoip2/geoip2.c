#include <ruby.h>
#include <maxminddb.h>

VALUE rb_mGeoIP2;
VALUE rb_cGeoIP2Database;
VALUE rb_cGeoIP2SearchResult;
VALUE rb_eGeoIP2Error;

static void mmdb_open(const char *db_path, MMDB_s *mmdb);
static void mmdb_close(MMDB_s *mmdb);
static bool mmdb_is_closed(MMDB_s *mmdb);
static VALUE mmdb_lookup(MMDB_s *mmdb, const char *ip_str, bool cleanup);

static void rb_geoip2_db_alloc(VALUE self);
static void rb_geoip2_db_free(MMDB_s *mmdb);
static VALUE rb_geoip2_db_initialize(VALUE self, VALUE path);
static VALUE rb_geoip2_db_close(VALUE self);
static VALUE rb_geoip2_db_lookup(VALUE self, VALUE ip);

static void search_result_free(void *pointer);
// static size_t search_result_size(void *pointer);

static const rb_data_type_t search_result_type = {
  "geoip2/search_result",
  {0, search_result_free, 0,},
};

static void
search_result_free(void *pointer)
{
  const MMDB_entry_data_list_s *entry_data_list;
  entry_data_list = pointer;
  MMDB_free_entry_data_list(entry_data_list);
}

static void
mmdb_open(const char *db_path, MMDB_s *mmdb)
{
  int status MMDB_open(db_path, MMDB_MODE_MMAP, mmdb);

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

static VALUE
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
                               &search_result_type,
                               entry_data_list);
}


static VALUE
rb_geoip2_db_alloc(VALUE self)
{
  MMDB_s *mmdb = RB_ALLOC(MMDB_s);
  return Data_Wrap_Struct(self, NULL, rb_geoip2_db_free, mmdb);
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
  Check_Type(path, T_STRING);

  char *db_path = StringValueCStr(path);
  MMDB_s *mmdb;

  Data_Get_Struct(self, MMDB_s, mmdb);
  mmdb_open(db_path, mmdb);

  return self;
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
  Check_Type(ip, T_STRING);
  char *ip_str = StringValueCStr(ip);
  MMDB_s *mmdb;

  Data_Get_Struct(self, MMDB_s, mmdb);
  VALUE search_retult = mmdb_lookup(mmdb, ip_str, false);

  return search_result;
}

static VALUE
rb_geoip2_sr_dig(int argc, VALUE *argv, VALUE self)
{
  VALUE arg;
  VALUE rest;
  rb_scan_args(argc, argv, "1*", &arg, &rest);
  Check_Type(arg, T_STRING);

  MMDB_entry_data_list_s *entry_data_list = NULL;
  TypedData_Get_Struct(self, MMDB_entry_data_list_s, &search_result_type, entry_data_list);

  char **path;
  int i = 0;
  path[i] = StringValueCStr(arg);
  while (true) {
    char *tmp = StringValueCStr(rb_ary_shift(rest));
    if (rb_ary_empty_p(rest) == Qtrue) {
      break;
    }
  }

  MMDB_entry_data_s entry_data = entry_data_list->entry_data;

  MMDB_aget_value(entry_data, path);
}


void
Init_geoip2(void)
{
  rb_mGeoIP2 = rb_define_module("GeoIP2");
  rb_cGeoIP2Database = rb_define_class_under(rb_mGeoIP2, "Database", rb_cObject);
  rb_cGeoIP2SearchResult = rb_define_class_under(rb_mGeoIP2, "SearchResult", rb_cData);
  rb_eGeoIP2Error = rb_define_class_under(rb_mGeoIP2, "Error", rb_eStandardError);
}
