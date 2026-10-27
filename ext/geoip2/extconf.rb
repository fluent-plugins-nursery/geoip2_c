require "mkmf"
require "mini_portile2"

maxminddb_version = "1.13.3"
maxminddb_sha256 = "a66502ea76eadbe17f2cd6fd708946777253972d2ae8157dee1b23a2fb528171"

recipe = MiniPortile.new("libmaxminddb", maxminddb_version)
recipe.files = [{
  url: "https://github.com/maxmind/libmaxminddb/releases/download/#{maxminddb_version}/libmaxminddb-#{maxminddb_version}.tar.gz",
  sha256: maxminddb_sha256
}]
recipe.cook
recipe.activate

$INCFLAGS = "-I#{File.join(recipe.path, 'include')} " + $INCFLAGS
$LDFLAGS << " -L#{File.join(recipe.path, 'lib')} -lmaxminddb"
$CFLAGS << " -std=c99 -fPIC -fms-extensions -I#{File.join(recipe.path, 'include')}"

have_func("rb_sym2str", "ruby.h")

create_makefile("geoip2/geoip2")
