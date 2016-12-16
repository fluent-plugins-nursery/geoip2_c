require "mkmf"

dir_config("maxminddb")
have_header("maxminddb.h")
have_library("maxminddb")

$CFLAGS << " -std=c99 -g -O0"

create_makefile("geoip2/geoip2")
