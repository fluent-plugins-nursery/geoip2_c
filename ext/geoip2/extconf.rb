require "mkmf"
require "rbconfig"

libdir = RbConfig::CONFIG["libdir"]
includedir = RbConfig::CONFIG["includedir"]

maxminddb_dir = File.expand_path(File.join(__dir__, "libmaxminddb"))

Dir.chdir(maxminddb_dir) do
  system("./bootstrap")
  system({ "CFLAGS" => "-fPIC" }, "./configure", "--disable-shared", "--disable-tests")
  system("make", "clean")
  system("make")
end

header_dirs = [includedir, "#{maxminddb_dir}/include"]
lib_dirs = [libdir, "#{maxminddb_dir}/src/.libs"]

dir_config("maxminddb", header_dirs, lib_dirs)
have_func("rb_sym2str", "ruby.h")

$LDFLAGS << " -L#{maxminddb_dir}/src/.libs -lmaxminddb"
$CFLAGS << " -std=c99 -fPIC -fms-extensions -I#{maxminddb_dir}/src/.libs"
# $CFLAGS << " -g -O0"

create_makefile("geoip2/geoip2")
