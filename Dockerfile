FROM ubuntu:latest
RUN mkdir /lpac
COPY . /lpac
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y libpcsclite-dev libcurl4-openssl-dev gcc make cmake unzip ninja-build php8.1 pkg-config
RUN cd /lpac/ && mkdir build && cd build && cmake .. -GNinja && ninja && cp /lpac/src/rlpa-server.php /lpac/build/output
WORKDIR /lpac/build/output
ENTRYPOINT php -f rlpa-server.php
