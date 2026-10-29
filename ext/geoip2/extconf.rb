require "mkmf"
require "mini_portile2"

maxminddb_version = "1.14.1"
maxminddb_sha256 = "ca5c87d41339f8bc4daabb53e8a9356b3c995f2d2419b85d7bff823b2ecc252d"

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

create_makefile("geoip2/geoip2")
