This folder is to deploy CRG in an R&D workflow.

In the R&D workflow, K8s will deploy a pod, containing all dependencies e.g. Mp-Spdz and Gramine, and it will also create a load-balancer service to communicate with the other cluster.

-Hari + Abhi

1. To deploy gramine based sgx tee for testing purposes with mutual attestation in a single container
 ```kubectl apply -f gramine-depl-mpspdz-tee.yaml```
2. To deploy  gramine based sgx tee for end-to-end mutual remote attestation 
    ``` kubectl apply -f gramine-depl-mpspdz-tee-remote.yaml 
         kubectl apply -f klyshko-lb-svc.yaml
    ```
3. Copying source code into container for testing
  ``` ## To test TEE mutual attestation in a single container
     ./copy_tee_script.sh
     ### To do  end-to-end mutual attestation across two containers
     ./copy_remote_script.sh
   ```
