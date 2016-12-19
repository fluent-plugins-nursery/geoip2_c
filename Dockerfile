FROM ruby:2.3.3-alpine

RUN apk --no-cache --update add build-base ruby-dev libc6-compat libmaxminddb-dev git

WORKDIR /app
COPY . .
