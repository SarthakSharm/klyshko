# -----------------------------------------------------------------------------
# Base image from Carbyne Stack SPDZ
# -----------------------------------------------------------------------------
    FROM ghcr.io/carbynestack/spdz:5350e66

    # -----------------------------------------------------------------------------
    # Metadata and Argument Definitions
    # -----------------------------------------------------------------------------
    ARG UBUNTU_CODENAME=focal
    ARG RELEASE_PAGE="https://github.com/carbynestack/klyshko/releases"
    ARG MBEDTLS_VERSION="3.6.2"
    
    # -----------------------------------------------------------------------------
    # Set environment variables for non-interactive installs
    # -----------------------------------------------------------------------------
    ENV DEBIAN_FRONTEND=noninteractive
    ENV SGX_AESM_ADDR=1
    
    # -----------------------------------------------------------------------------
    # Copy Configuration Files
    # -----------------------------------------------------------------------------
    COPY azure_sgx_qcnl.conf /etc/sgx_default_qcnl.conf
    COPY restart_aesm.sh /restart_aesm.sh
    
    # -----------------------------------------------------------------------------
    # Install Required Dependencies
    # -----------------------------------------------------------------------------
    RUN apt-get update && apt-get install -y \
        curl \
        bzip2 \
        cmake \
        gnupg2 \
        binutils \
        build-essential \
        autoconf \
        bison \
        gawk \
        ninja-build \
        pkg-config \
        python3 \
        python3-pip \
        python3-cryptography \
        python3-protobuf \
        python3-click \
        python3-jinja2 \
        python3-pyelftools \
        git \
        make \
        vim \
        wget \
        psmisc \
        busybox \
        clang \
        cpio \
        dwarves \
        g++ \
        gcc \
        gdb \
        jq \
        kmod \
        libevent-dev \
        libmemcached-tools \
        libomp-dev \
        libssl-dev \
        libunwind8 \
        musl-tools \
        ncat \
        nginx \
        python3-numpy \
        python3-pytest \
        python3-pytest-xdist \
        python3-scipy \
        qemu-kvm \
        shellcheck \
        sqlite3 \
        zlib1g-dev && \
        apt-get clean && \
        rm -rf /var/lib/apt/lists/*
    
    # -----------------------------------------------------------------------------
    # Install Gramine and Intel SGX Packages
    # -----------------------------------------------------------------------------
    RUN curl -fsSLo /usr/share/keyrings/gramine-keyring.gpg https://packages.gramineproject.io/gramine-keyring.gpg && \
        echo "deb [arch=amd64 signed-by=/usr/share/keyrings/gramine-keyring.gpg] https://packages.gramineproject.io/ ${UBUNTU_CODENAME} main" > /etc/apt/sources.list.d/gramine.list && \
        curl -fsSL https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | gpg --dearmor -o /usr/share/keyrings/intel-sgx.gpg && \
        echo "deb [arch=amd64 signed-by=/usr/share/keyrings/intel-sgx.gpg] https://download.01.org/intel-sgx/sgx_repo/ubuntu ${UBUNTU_CODENAME} main" > /etc/apt/sources.list.d/intel-sgx.list && \
        apt-get update && apt-get install -y \
        gramine \
        sgx-aesm-service \
        libsgx-aesm-launch-plugin \
        libsgx-aesm-epid-plugin \
        libsgx-aesm-quote-ex-plugin \
        libsgx-aesm-ecdsa-plugin \
        libsgx-dcap-quote-verify \
        libprotobuf-c-dev \
        protobuf-c-compiler \
        python3-cryptography \
        python3-pip \
        python3-protobuf \
        libsgx-dcap-quote-verify-dev && \
        apt-get clean && \
        rm -rf /var/lib/apt/lists/*
    
        # RUN apt update && apt install -y \
        # git make gcc wget libsgx-dcap-ql libsgx-quote-ex libsgx-dcap-default-qpl pkg-config vim
    # -----------------------------------------------------------------------------
    # Install mbedTLS
    # -----------------------------------------------------------------------------
    RUN curl -fsSLo /tmp/mbedtls-${MBEDTLS_VERSION}.tar.bz2 https://packages.gramineproject.io/distfiles/mbedtls-${MBEDTLS_VERSION}.tar.bz2 && \
        cd /tmp && \
        tar xjf mbedtls-${MBEDTLS_VERSION}.tar.bz2 && \
        cd mbedtls-${MBEDTLS_VERSION} && \
        mkdir build && cd build && \
        cmake .. && \
        make && \
        make install && \
        rm -rf /tmp/mbedtls-${MBEDTLS_VERSION}*
    
    # -----------------------------------------------------------------------------
    # Install protobuf
    # -----------------------------------------------------------------------------
    RUN curl -fsSLo /tmp/libprotobuf23_3.12.4-1_amd64.deb https://packages.gramineproject.io/pool/libprotobuf/p/protobuf/libprotobuf23_3.12.4-1_amd64.deb && \
        dpkg -i /tmp/libprotobuf23_3.12.4-1_amd64.deb && \
        rm /tmp/libprotobuf23_3.12.4-1_amd64.deb
    
    # -----------------------------------------------------------------------------
    # Install Python Dependencies for Gramine
    # -----------------------------------------------------------------------------
    RUN python3 -m pip install 'meson>=0.56' 'tomli>=1.1.0' 'tomli-w>=0.4.0'
    
    # -----------------------------------------------------------------------------
    # Prepare Gramine Environment
    # -----------------------------------------------------------------------------
    RUN mkdir -p /var/run/aesmd/ && \
        rm -rf Player-Data && \
        mkdir -p Player-Data/2-2-40 && \
        mkdir -p Player-Data/2-p-128
    
    # -----------------------------------------------------------------------------
    # Generate Gramine Private Key
    # -----------------------------------------------------------------------------
    RUN gramine-sgx-gen-private-key

    
COPY 3RD-PARTY-LICENSES /3RD-PARTY-LICENSES
COPY final_source_code/azure_sgx_qcnl.conf .
COPY final_source_code/client_run.sh .
COPY final_source_code/client.manifest.template .
COPY final_source_code/Makefile .   
COPY final_source_code/server.manifest.template .
COPY final_source_code/ssl ./ssl/
COPY final_source_code/test2 ./test2/
RUN cp libSPDZ.so /usr/local/lib/

    # -----------------------------------------------------------------------------
    # Set Up Gramine Environment and Manifest
    #COPY gramine ./gramine/

    # Ensure Meson build configuration works properly
    # RUN mkdir -p /gramine/build && \
    #     meson setup /gramine/build /gramine --buildtype=release -Ddcap=enabled && \
    #     cd /gramine && make app RA_TYPE=dcap && \
    #     gramine-manifest -Dlog_level=error file_2.manifest.template file_2.manifest && \
    #     gramine-sgx-sign --manifest file_2.manifest --output file_2.manifest.sgx
    # # -----------------------------------------------------------------------------
    # Add 3rd-Party License Information
    # -----------------------------------------------------------------------------
    RUN printf "\n## Klyshko MP-SPDZ\n\
    General information about third-party software components and their licenses, \
    which are distributed with Klyshko MP-SPDZ, can be found in the \
    [SBOM](./sbom.json). Further details are available in the subfolder for the \
    respective component or can be downloaded from the \
    [Klyshko Release Page](%s).\n" "${RELEASE_PAGE}" \
    >> /3RD-PARTY-LICENSES/disclosure.md
    
    # -----------------------------------------------------------------------------
    # Restart AESM Service
    # -----------------------------------------------------------------------------
    RUN bash /restart_aesm.sh
    
    # ---------------------------------------------------------------------------
    
    # -----------------------------------------------------------------------------
    # Default Command
    # -----------------------------------------------------------------------------
    CMD ["/bin/bash"]
    