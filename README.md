# GeoIP2

This gem provides binding of [libmaxminddb](http://maxmind.github.io/libmaxminddb/).

This binding does not traverse all elements in lookup result by default.
So you can get the element you want fast such as city name, country name or etc.

## Requirements

* Ruby 2.3 or later
* libmaxminddb

NOTE:

This library uses `Hash#dig` and `Array#dig`.
If you want to use this library with Ruby2.1 or Ruby2.2, you can install a gem which adds support `#dig` method.
But I don't want to add extra dependency to this library, so I decided not to add extra dependency to support old Ruby versions.

### libmaxminddb

* Debian: [libmaxminddb](https://packages.debian.org/search?keywords=libmaxminddb&searchon=names&suite=all&section=all)
* Ubuntu: [Ubuntu official package](http://packages.ubuntu.com/search?keywords=libmaxminddb&searchon=names&suite=all&section=all) or [PPA of MaxMind team](https://launchpad.net/~maxmind/+archive/ubuntu/ppa)
* CentOS/Fedora: [rpms/libmaxminddb](https://admin.fedoraproject.org/pkgdb/package/rpms/libmaxminddb/)

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'geoip2_c'
```

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install geoip2_c

## Usage

```ruby
require "geoip2"

db = GeoIP2::Databaes.new("/path/to/GeoLite2-City.mmdb")
result = db.lookup("66.102.9.80")
retult.get_value("city", "names", "en") # => "Mountain View"
retult.dig("city", "names", "en")       # => "Mountain View"
```

## Development

After checking out the repo, run `bundle install` to install dependencies. Then, run `rake test` to run the tests.

To install this gem onto your local machine, run `bundle exec rake install`. To release a new version, update the version number in `version.rb`, and then run `bundle exec rake release`, which will create a git tag for the version, push git commits and tags, and push the `.gem` file to [rubygems.org](https://rubygems.org).

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/okkez/geoip2_c.

## License

The gem is available as open source under the terms of the [Apache-2.0](http://opensource.org/licenses/Apache-2.0).

