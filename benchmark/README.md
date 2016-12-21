# Benchmark

* C extension for libmaxminddb
  * maxmind_geoip2
  * hive_geoip2
  * geoip2_compat: API is compatible with geoip-c
  * geoip2_c: This gem
* GeoLite legacy database
  * geoip-c
* Pure Ruby implementation to parse GeoIP2 database
  * maxminddb

## Scenario

Lookup 10,000 random IPv4 addresses

## Result

```
$ bundle exec ruby bench.rb
Rehearsal ---------------------------------------------------------
geoip(legacy format)    0.050000   0.000000   0.050000 (  0.052915)
geoip2_compat           0.040000   0.000000   0.040000 (  0.035988)
maxminddb (pure ruby)   2.180000   0.000000   2.180000 (  2.185618)
hive                    0.180000   0.000000   0.180000 (  0.175035)
maxmind_geoip2          0.520000   0.270000   0.790000 (  0.782362)
geoip2_c                0.000000   0.000000   0.000000 (  0.005785)
geoip2_c(#to_h)         0.190000   0.000000   0.190000 (  0.186337)
geoip2_c(8 keys)        0.050000   0.000000   0.050000 (  0.050778)
------------------------------------------------ total: 3.480000sec

                            user     system      total        real
geoip(legacy format)    0.040000   0.000000   0.040000 (  0.041069)
geoip2_compat           0.030000   0.000000   0.030000 (  0.026809)
maxminddb (pure ruby)   2.210000   0.000000   2.210000 (  2.212734)
hive                    0.180000   0.000000   0.180000 (  0.184725)
maxmind_geoip2          0.570000   0.210000   0.780000 (  0.781827)
geoip2_c                0.000000   0.000000   0.000000 (  0.005813)
geoip2_c(#to_h)         0.180000   0.000000   0.180000 (  0.179669)
geoip2_c(8 keys)        0.040000   0.000000   0.040000 (  0.047905)
```

## How to run

```
$ ./setup.sh
$ bundle install
$ bundle exec ruby bench.rb 2> /dev/null
```
