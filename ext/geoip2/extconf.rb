require "mkmf"
require "mini_portile2"

maxminddb_version = "1.13.3"

recipe = MiniPortile.new("libmaxminddb", maxminddb_version)
recipe.files = ["https://github.com/maxmind/libmaxminddb/releases/download/#{maxminddb_version}/libmaxminddb-#{maxminddb_version}.tar.gz"]
recipe.cook
recipe.activate

$INCFLAGS = "-I#{File.join(recipe.path, 'include')} " + $INCFLAGS
$LDFLAGS << " -L#{File.join(recipe.path, 'lib')} -lmaxminddb"
$CFLAGS << " -std=c99 -fPIC -fms-extensions -I#{File.join(recipe.path, 'include')}"

have_func("rb_sym2str", "ruby.h")

create_makefile("geoip2/geoip2")
