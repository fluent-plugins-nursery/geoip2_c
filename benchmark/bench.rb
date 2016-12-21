require "benchmark"
require "ipaddr"
require "profiler"

require "geoip"
require "geoip2_compat"
require "maxminddb"
require "hive_geoip2"
require "maxmind_geoip2"
require "geoip2"
require "pp"

def random_ip_addresses(address_mask, n = 10)
  IPAddr.new(address_mask).to_range.to_a.sample(n).map(&:to_s)
end

ips = []
# generate 10000 IP addresses
1000.times do
  ips.concat(random_ip_addresses("#{rand(256)}.#{rand(256)}.#{rand(256)}.0/24"))
end

geoip = GeoIP::City.new("./GeoLiteCity.dat", :memory, false)
# geoip = GeoIP.new("./GeoLiteCity.dat")
geoip2_compat = GeoIP2Compat.new("./GeoLite2-City.mmdb")
mmdb = MaxMindDB.new("./GeoLite2-City.mmdb")
hive = Hive::GeoIP2.new("./GeoLite2-City.mmdb")
MaxmindGeoIP2.file("./GeoLite2-City.mmdb")
MaxmindGeoIP2.locale("ja")
geoip2_c = GeoIP2::Database.new("./GeoLite2-City.mmdb")

Benchmark.bmbm do |x|
  if geoip.respond_to?(:look_up)
    x.report("geoip(legacy format)") do
      ips.each do |ip|
        geoip.look_up(ip)
      end
    end
  else
    x.report("geoip(legacy format)(ruby)") do
      ips.each do |ip|
        geoip.city(ip)
      end
    end
  end
  x.report("geoip2_compat") do
    ips.each do |ip|
      geoip2_compat.lookup(ip)
    end
  end
  x.report("maxminddb (pure ruby)") do
    ips.each do |ip|
      mmdb.lookup(ip)
    end
  end
  x.report("hive") do
    ips.each do |ip|
      hive.lookup(ip)
    end
  end
  x.report("maxmind_geoip2") do
    ips.each do |ip|
      MaxmindGeoIP2.locate(ip)
    end
  end
  x.report("geoip2_c") do
    ips.each do |ip|
      geoip2_c.lookup(ip)
    end
  end
  x.report("geoip2_c(#to_h)") do
    ips.each do |ip|
      r = geoip2_c.lookup(ip)
      if r
        r.to_h
      end
    end
  end
  x.report("geoip2_c(8 keys)") do
    ips.each do |ip|
      r = geoip2_c.lookup(ip)
      if r
        r.dig("country", "iso_code")
        r.dig("country", "names", "en")
        # r.dig("subdivisions", "0", "iso_code")
        # r.dig("subdivisions", "0", "names", "en")
        r.dig("city", "names", "en")
        r.dig("postal", "code")
        r.dig("location", "latitude")
        r.dig("location", "longtitude")
      end
    end
  end
end

hive.close
geoip2_c.close
