FROM ghcr.io/carbynestack/spdz:5350e66
#FROM ubuntu

RUN apt-get update \
    && env DEBIAN_FRONTEND=noninteractive apt-get install -y \
    curl \
    gnupg2 \
    wget

RUN wget https://packages.microsoft.com/ubuntu/20.04/prod/pool/main/a/az-dcap-client/az-dcap-client_1.12.3_amd64.deb \ 
    && dpkg -i az-dcap-client_1.12.3_amd64.deb

WORKDIR /kii 

COPY ssl .

COPY KII /usr/local/bin

RUN echo 'deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu focal main' \
    > /etc/apt/sources.list.d/intel-sgx.list \
    && wget https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key \
    && apt-key add intel-sgx-deb.key

RUN curl -fsSLo /usr/share/keyrings/gramine-keyring.gpg https://packages.gramineproject.io/gramine-keyring.gpg
RUN echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/gramine-keyring.gpg] https://packages.gramineproject.io/ stable main' | tee /etc/apt/sources.list.d/gramine.list
RUN apt-get update
RUN apt-get install -y gramine-dcap
RUN apt-get install -y gramine

COPY x86_64-linux-gnu /usr/lib/x86_64-linux-gnu-2/

RUN export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/x86_64-linux-gnu-2/
# ENTRYPOINT ["KII"]