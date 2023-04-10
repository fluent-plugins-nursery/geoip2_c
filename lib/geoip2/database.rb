module GeoIP2
  class Database
    def initialize(path, symbolize_keys: false)
      @symbolize_keys = !!symbolize_keys
      open_mmdb(path)
    end
  end
end
