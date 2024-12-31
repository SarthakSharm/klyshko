#!/bin/bash

echo Running Verifier i.e. KII for 1-party local attestation test

# Configuration Variables
export KII_TUPLES_PER_JOB="100000"
export KII_SHARED_FOLDER="/kii"
export KII_TUPLE_FILE="/kii/tuples"
export KII_PLAYER_COUNT="1"
export KII_JOB_ID="1920bb26-dsee-dzfw-vdsdsa14fds4"
export KII_TUPLE_TYPE="BIT_GFP"
export KII_PLAYER_ENDPOINT_1="127.0.0.1:1025"
export KII_PLAYER_ENDPOINT_0="127.0.0.1:1026"
export BASE_PORT="4433"

# Run make with SGX and RA_TYPE as build variables
echo compiling Application with DCAP attestation
make app RA_TYPE=dcap
echo ..done.

# Retrieve mr_enclave and mr_signer values from server.sig
output=$(gramine-sgx-sigstruct-view server.sig)
mr_enclave=$(echo "$output" | grep "mr_enclave" | awk '{print $2}')
mr_signer=$(echo "$output" | grep "mr_signer" | awk '{print $2}')

echo Retrieving trusted MRENCLAVE and MRSIGNER from build process
echo "mr_enclave: $mr_enclave, mr_signer: $mr_signer, i: $i"

# Check if mr_enclave and mr_signer are correctly retrieved
if [ -z "$mr_enclave" ] || [ -z "$mr_signer" ]; then
    echo "Error: Could not retrieve mr_enclave or mr_signer from server.sig"
    exit 1
fi

echo Initializing RA-TLS environment variables.
# Set required RA-TLS verification variables
export RA_TLS_MRSIGNER="$mr_signer"  
export RA_TLS_MRENCLAVE="$mr_enclave"            
export RA_TLS_ISV_SVN="any"
export RA_TLS_ISV_PROD_ID="any"

export KII_PLAYER_NUMBER=$i
export RA_TLS_ALLOW_DEBUG_ENCLAVE_INSECURE=1
export RA_TLS_ALLOW_OUTDATED_TCB_INSECURE=1
export RA_TLS_ALLOW_HW_CONFIG_NEEDED=1
export RA_TLS_ALLOW_SW_HARDENING_NEEDED=1

./KII "$mr_enclave" "$mr_signer" 0 0 0
echo DONE
