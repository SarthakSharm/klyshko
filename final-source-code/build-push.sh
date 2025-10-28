#!/bin/bash

COMMIT_ID=`git log  -1 --pretty=%h`
# last commit id of master branch
# To be executed from project root
docker build -t ghcr.io/datakaveri/gramine-base:mpspdz-tee-$COMMIT_ID -f Dockerfile.tee-fake-offline  . && \
docker push ghcr.io/datakaveri/gramine-base:mpspdz-tee-$COMMIT_ID
