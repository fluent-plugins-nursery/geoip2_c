require "mkmf"

dir_config("maxminddb")
have_header("maxminddb.h")
have_library("maxminddb")

$CFLAGS << " -std=c99"

create_makefile("geoip2/geoip2")
