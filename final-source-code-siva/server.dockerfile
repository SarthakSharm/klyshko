FROM ghcr.io/carbynestack/spdz:5350e66

RUN apt-get update \
    && env DEBIAN_FRONTEND=noninteractive apt-get install -y wget \
    build-essential \
    gnupg2 \
    libcurl3-gnutls \
    python3

RUN apt-get update \
    && apt-get install -y libsgx-urts \
    libsgx-dcap-ql \
    libsgx-quote-ex

WORKDIR /server

COPY ssl .

COPY server /usr/local/bin

ENTRYPOINT ["server"]