#
# Copyright (c) 2023 - for information on the respective copyright owner
# see the NOTICE file and/or the repository https://github.com/carbynestack/klyshko.
#
# SPDX-License-Identifier: Apache-2.0
#

FROM ghcr.io/carbynestack/spdz:5350e66
ARG UBUNTU_CODENAME=focal

ARG RELEASE_PAGE="https://github.com/carbynestack/klyshko/releases"

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y curl gnupg2 binutils git make gcc  wget 

RUN curl -fsSLo /usr/share/keyrings/gramine-keyring.gpg https://packages.gramineproject.io/gramine-keyring.gpg && \
    echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/gramine-keyring.gpg] https://packages.gramineproject.io/ '${UBUNTU_CODENAME}' main' > /etc/apt/sources.list.d/gramine.list

RUN curl -fsSLo /usr/share/keyrings/intel-sgx-deb.key https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key && \
    echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/intel-sgx-deb.key] https://download.01.org/intel-sgx/sgx_repo/ubuntu '${UBUNTU_CODENAME}' main' > /etc/apt/sources.list.d/intel-sgx.list

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y gramine \
    sgx-aesm-service \
    libsgx-aesm-launch-plugin \
    libsgx-aesm-epid-plugin \
    libsgx-aesm-quote-ex-plugin \
    libsgx-aesm-ecdsa-plugin \
    libsgx-dcap-quote-verify \
    psmisc && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /var/run/aesmd/

COPY restart_aesm.sh /restart_aesm.sh

RUN apt update && apt install -y  libsgx-dcap-ql libsgx-quote-ex libsgx-dcap-default-qpl pkg-config vim 
ENV SGX_AESM_ADDR=1

COPY azure_sgx_qcnl.conf /etc/sgx_default_qcnl.conf


# Copy resources
COPY 3RD-PARTY-LICENSES /3RD-PARTY-LICENSES
COPY kii-run.sh .
COPY file_2.cpp .
COPY file_2.manifest.template .
RUN gramine-sgx-gen-private-key

RUN gcc -o file_2 file_2.cpp

RUN printf "\n## Klyshko MP-SPDZ\n\
General information about third-party software components and their licenses, \
which are distributed with Klyshko MP-SPDZ, can be found in the \
[SBOM](./sbom.json). Further details are available in the subfolder for the \
respective component or can be downloaded from the \
[Klyshko Release Page](%s).\n" "${RELEASE_PAGE}"\
  >> /3RD-PARTY-LICENSES/disclosure.md

# Run the kii-run script
ENTRYPOINT ["/bin/bash", "-c", "gramine-sgx ./file_2"]

