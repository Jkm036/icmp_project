#! /bin/bash

# Some variables and that
IMAGE_NAME="node-template"
NETWORK_NAME=ping_net
NODE_NAME_ONE="node1"
NODE_NAME_TWO="node2"
# script wil exit on an error 
 
if ! docker network inspect ${NETWORK_NAME} >/dev/null 2>&1; then 
	echo "Creating network named ${NETWORK_NAME}"
	if ! docker network create --subnet=172.16.0.0/16 ${NETWORK_NAME}; then
		echo "There was a problem creating the network"
		exit 1
	fi
fi 

# Cleaning up stuff that's already been done
cleanup(){
	echo "Cleaning containers..."
 	docker rm -f ${NODE_NAME_ONE} ${NODE_NAME_TWO} 2>/dev/null 
	echo "Done"
}
# Trap terminating signals and call this
trap cleanup EXIT 

# Create containers on network for node test
create_container(){
	if [ $# -ne 2 ]; then
		echo "Usage: create_container <name> <IP address>"
		exit 1
	fi
	if ! docker image inspect ${IMAGE_NAME} > /dev/null 2>&1; then 
		echo "Image not found"
		exit 1
	fi

	docker run -d  --name ${1} --network ${NETWORK_NAME} --cap-add=NET_RAW --cap-add=NET_ADMIN --ip ${2} ${IMAGE_NAME}

	if [ $? -ne 0 ]; then 
		echo "Something went wrong with creating containers.."
		echo "Couldn't create ping_test"
		exit 1
	fi 
	return 0
}


echo "Creating containers"
create_container ${NODE_NAME_ONE} "172.16.0.2"
create_container ${NODE_NAME_TWO} "172.16.0.3"


cat << EOF
Containers are ready!
To test joshping from ${NODE_NAME_ONE} to ${NODE_NAME_TWO}:
docker exec ${NODE_NAME_ONE} ./joshping -c 4 172.16.0.3

To test joshping from ${NODE_NAME_TWO} to ${NODE_NAME_ONE}:
docker exec ${NODE_NAME_TWO} ./joshping -c 4 172.16.0.2

To enter ${NODE_NAME_ONE}:
docker exec -it ${NODE_NAME_ONE} bash

To enter ${NODE_NAME_TWO}:
docker exec -it ${NODE_NAME_TWO} bash

Press Ctrl+C to cleanup and

EOF

# Keep script running for whatever reason
while true; do
   sleep 1 
done
