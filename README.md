# GeoIP2

This gem provides binding of [libmaxminddb](http://maxmind.github.io/libmaxminddb/).

This binding does not traverse all elements in lookup result by default.
So you can get the element you want fast such as city name, country name or etc.

## Supported Ruby versions

See [workflows](https://github.com/fluent-plugins-nursery/geoip2_c/blob/master/.github/workflows/ubuntu.yml)

## Build requirements

Need ruby.h to build this extension, so you must install it before build this extension.

Debian GNU Linux / Ubuntu:

```
$ sudo apt install -y build-essential automake autoconf libtool
```

CentOS:

```
$ sudo yum groupinstall -y "Development Tools"
```

**NOTE**: If you want to use libmaxminddb provided as a deb/rpm package, you can install libmaxminddb-dev or libmaxminddb-devel at your own risk.

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

db = GeoIP2::Database.new("GeoLite2-City.mmdb")
result = db.lookup("66.102.9.80")
result.get_value("city", "names", "en") # => "Mountain View"
result.dig("city", "names", "en")       # => "Mountain View"
```

## Development

After checking out the repo, run `bundle install` to install dependencies. Then, run `rake test` to run the tests.

To install this gem onto your local machine, run `bundle exec rake install`. To release a new version, update the version number in `version.rb`, and then run `bundle exec rake release`, which will create a git tag for the version, push git commits and tags, and push the `.gem` file to [rubygems.org](https://rubygems.org).

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/fluent-plugins-nursery/geoip2_c.

## License

The gem is available as open source under the terms of the [Apache-2.0](http://opensource.org/licenses/Apache-2.0).

