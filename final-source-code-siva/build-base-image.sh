/bin/sh install_gramine.sh

# Create kii image
make clean && make KII

docker build -f kii.dockerfile -t kii-img .

# Create server image
make clean && make server

docker build -f server.dockerfile -t server-img .
