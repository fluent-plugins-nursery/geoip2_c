require "mkmf"

# By default, this downloads libmaxminddb and builds it. To link the
# libmaxminddb installed on the system instead, for example where the build
# cannot access github.com, pass --use-system-libraries
# (gem install geoip2_c -- --use-system-libraries) or set
# GEOIP2_C_USE_SYSTEM_LIBRARIES.
if arg_config("--use-system-libraries", ENV.key?("GEOIP2_C_USE_SYSTEM_LIBRARIES"))
  pkg_config("libmaxminddb")
  unless have_header("maxminddb.h") && have_library("maxminddb", "MMDB_open")
    abort "libmaxminddb is not found. Install libmaxminddb-dev (Debian, Ubuntu) " \
          "or libmaxminddb-devel (RHEL), or build without --use-system-libraries " \
          "and without GEOIP2_C_USE_SYSTEM_LIBRARIES."
  end
  $CFLAGS << " -std=c99 -fPIC -fms-extensions"
else
  require "mini_portile2"

  maxminddb_version = "1.14.1"
  maxminddb_sha256 = "ca5c87d41339f8bc4daabb53e8a9356b3c995f2d2419b85d7bff823b2ecc252d"
  maxminddb_url = "https://github.com/maxmind/libmaxminddb/releases/download/#{maxminddb_version}/libmaxminddb-#{maxminddb_version}.tar.gz"

  recipe = MiniPortile.new("libmaxminddb", maxminddb_version)
  recipe.files = [{
    url: maxminddb_url,
    sha256: maxminddb_sha256
  }]
  recipe.configure_options += ["--with-pic"]
  begin
    recipe.download
  rescue
    raise if recipe.downloaded?
    abort <<~MESSAGE
      Failed to download #{maxminddb_url}.
      If the build accesses github.com through a proxy, set https_proxy.
      To link libmaxminddb installed on the system instead, install libmaxminddb-dev (Debian, Ubuntu)
      or libmaxminddb-devel (RHEL), and pass --use-system-libraries
      (gem install geoip2_c -- --use-system-libraries) or set GEOIP2_C_USE_SYSTEM_LIBRARIES.
    MESSAGE
  end
  recipe.cook
  recipe.activate

  $INCFLAGS = "-I#{File.join(recipe.path, 'include')} " + $INCFLAGS
  $LDFLAGS << " -L#{File.join(recipe.path, 'lib')} -lmaxminddb"
  $CFLAGS << " -std=c99 -fPIC -fms-extensions -I#{File.join(recipe.path, 'include')}"
end

create_makefile("geoip2/geoip2")
