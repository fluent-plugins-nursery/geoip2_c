$LOAD_PATH.unshift File.expand_path('../../lib', __FILE__)
require "geoip2"
require "test-unit"
require "pathname"
require "ipaddr"

Bundler.require(:default)

def base_dir
  Pathname(__dir__)
end

def mmdb_test_data_dir
  base_dir + "MaxMind-DB/test-data"
end

def mmdb_source_data_dir
  base_dir + "MaxMind-DB/source-data"
end

def mmdb_test_data(name)
  (mmdb_test_data_dir + name).to_s
end

def mmdb_source_data(name)
  mmdb_source_data_dir + name
end

def random_ip_addresses(address_mask, n = 10)
  IPAddr.new(address_mask).to_range.lazy.select {
    rand(1000) % 7 == 0
  }.first(n * 100).sample(n)
end

def random_ip_data(address_mask, n = 10)
  hash = {}
  random_ip_addresses(address_mask, n).each do |ip|
    hash[ip.to_s] = ip.to_s
  end
  hash
end
