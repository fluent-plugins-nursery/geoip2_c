#!/bin/bash

curl -L http://geolite.maxmind.com/download/geoip/database/GeoLite2-City.mmdb.gz | \
    gunzip -c > GeoLite2-City.mmdb
curl -L http://geolite.maxmind.com/download/geoip/database/GeoLiteCity.dat.gz | \
    gunzip -c > GeoLiteCity.dat
