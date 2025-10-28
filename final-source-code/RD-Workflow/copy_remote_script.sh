#!/bin/bash
set  -exu
pod=`kubectl get pods | grep gramine-mpspdz-tee-remote| cut -d ' ' -f 1`
kubectl cp ../../final-source-code2/ "$pod":/mp-spdz-5350e66
echo " copied final-source-code2 to $pod"
