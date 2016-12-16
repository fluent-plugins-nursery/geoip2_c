module GeoIP2
  class LookupResult
    def dig(*keys)
      get_value(*keys)
    end
  end
end
