FROM ruby:2.3.3-alpine

RUN apk --no-cache --update add build-base ruby-dev libmaxminddb-dev git
