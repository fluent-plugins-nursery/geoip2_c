require "helper"

class GeoIP2Test < Test::Unit::TestCase
  data do
    hash = {}
    mmdb_test_data_dir.each_child do |entry|
      next unless entry.extname == ".mmdb"
      # GeoIP2-City-Test-Invalid-Node-Count.mmdb causes SEGV
      next if /invalid/i =~ entry.basename.to_s
      hash[entry.basename.to_s] = entry.to_s
    end
    hash
  end
  test "open all files" do |filename|
    assert_nothing_raised do
      db = GeoIP2::Database.new(filename)
      db.close
    end
  end

  sub_test_case "city" do
    setup do
      @db = GeoIP2::Database.new(mmdb_test_data("GeoIP2-City-Test.mmdb"))
    end

    teardown do
      @db.close
    end

    data do
      random_ip_data("::81.2.69.142/127")
    end
    test "London" do |ip|
      result = @db.lookup(ip)
      expected = {
        city_name: "London",
        city_geoname_id: 2643743,
        continent_code: "EU",
      }
      actual = {
        city_name: result.get_value("city", "names", "en"),
        city_geoname_id: result.get_value("city", "geoname_id"),
        continent_code: result.get_value("continent", "code"),
      }
      assert_equal(expected, actual)
    end

    test "city.names is a Hash" do
      result = @db.lookup("81.2.69.142")
      assert_instance_of(Hash, result.get_value("city", "names"))
    end

    test "subdivisions is an Array" do
      result = @db.lookup("81.2.69.142")
      assert_instance_of(Array, result.get_value("subdivisions"))
    end

    data do
      random_ip_data("127.0.0.0/24")
        .merge(random_ip_data("192.168.0.0/24"))
    end
    test "cannot find IPv4 private address" do |ip|
      assert_nil(@db.lookup(ip))
    end
  end

  sub_test_case "anonymous" do
    setup do
      @db = GeoIP2::Database.new(mmdb_test_data("GeoIP2-Anonymous-IP-Test.mmdb"))
    end

    teardown do
      @db.close
    end

    data do
      random_ip_data("::1.2.0.0/112", 10)
    end
    test "vpn" do |ip|
      result = @db.lookup(ip)
      expected = {
        "is_anonymous" => true,
        "is_anonymous_vpn" => true
      }
      assert_equal(expected, result.to_h)
    end

    data do
      random_ip_data("::65.0.0.0/109", 10)
    end
    test "tor" do |ip|
      result = @db.lookup(ip)
      expected = {
        "is_anonymous" => true,
        "is_tor_exit_node" => true
      }
      assert_equal(expected, result.to_h)
    end
  end

  sub_test_case "connection type" do
    setup do
      @db = GeoIP2::Database.new(mmdb_test_data("GeoIP2-Connection-Type-Test.mmdb"))
    end

    teardown do
      @db.close
    end

    data do
      random_ip_data("::1.0.1.0/120")
        .merge(random_ip_data("::96.1.0.0/112"))
    end
    test "Cable/DSL" do |ip|
      result = @db.lookup(ip)
      expected = { "connection_type" => "Cable/DSL" }
      assert_equal(expected, result.to_h)
    end

    data do
      random_ip_data("::1.0.32.0/115", 10)
        .merge(random_ip_data("::1.0.128.0/113"))
    end
    test "Dialup" do |ip|
      result = @db.lookup(ip)
      expected = { "connection_type" => "Dialup" }
      assert_equal(expected, result.to_h)
    end

    data do
      random_ip_data("::207.179.48.0/116")
        .merge(random_ip_data("::108.96.0.0/107"))
    end
    test "Cellular" do |ip|
      result = @db.lookup(ip)
      expected = { "connection_type" => "Cellular" }
      assert_equal(expected, result.to_h)
    end
  end

  sub_test_case "Country" do
    setup do
      @db = GeoIP2::Database.new(mmdb_test_data("GeoIP2-Country-Test.mmdb"))
    end

    teardown do
      @db.close
    end

    data do
      random_ip_data("2001:218::/32")
        .merge(random_ip_data("2001:240::/32"))
    end
    test "Japan" do |ip|
      result = @db.lookup(ip)
      expected = {
        continent_code: "AS",
        country_iso_code: "JP",
        country_name_en: "Japan"
      }
      actual = {
        continent_code: result.get_value("continent", "code"),
        country_iso_code: result.get_value("country", "iso_code"),
        country_name_en: result.get_value("country", "names", "en")
      }
      assert_equal(expected, actual)
    end

    data do
      random_ip_data("::89.160.20.128/121")
    end
    test "Sweden" do |ip|
      result = @db.lookup(ip)
      expected = {
        continent_code: "EU",
        country_iso_code: "SE",
        country_name_en: "Sweden"
      }
      actual = {
        continent_code: result.get_value("continent", "code"),
        country_iso_code: result.get_value("country", "iso_code"),
        country_name_en: result.get_value("country", "names", "en")
      }
      assert_equal(expected, actual)
    end
  end
end
