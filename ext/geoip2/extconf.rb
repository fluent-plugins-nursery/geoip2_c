require "mkmf"

dir_config("maxminddb")
have_header("maxminddb.h")
have_library("maxminddb")
have_func("rb_sym2str", "ruby.h")

$CFLAGS << " -std=c99"
# $CFLAGS << " -g -O0"

create_makefile("geoip2/geoip2")
