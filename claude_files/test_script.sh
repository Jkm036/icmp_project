#!/bin/bash

# Exit on any error
set -e

# Create custom network if it doesn't exist
if ! docker network inspect ping_net >/dev/null 2>&1; then
    echo "Creating custom network..."
    docker network create --subnet=172.20.0.0/16 ping_net
fi

# Build the Docker image
echo "Building Docker image..."
cat > Dockerfile << 'EOF'
FROM gcc:latest
WORKDIR /app
COPY icmp_ping.c .
RUN gcc -o ping icmp_ping.c -lpthread
CMD ["tail", "-f", "/dev/null"]
EOF

docker build -t ping-test .

# Function to clean up containers
cleanup() {
    echo "Cleaning up containers..."
    docker rm -f container1 container2 2>/dev/null || true
    echo "Done"
}

# Trap Ctrl+C and call cleanup
trap cleanup EXIT

# Create containers
echo "Creating containers..."
docker run -d \
    --name container1 \
    --network ping_net \
    --ip 172.20.0.2 \
    --cap-add=NET_RAW \
    --cap-add=NET_ADMIN \
    ping-test

docker run -d \
    --name container2 \
    --network ping_net \
    --ip 172.20.0.3 \
    --cap-add=NET_RAW \
    --cap-add=NET_ADMIN \
    ping-test

echo "Containers are ready!"
echo "To test ping from container1 to container2:"
echo "docker exec container1 ./ping -c 4 172.20.0.3"
echo
echo "To test ping from container2 to container1:"
echo "docker exec container2 ./ping -c 4 172.20.0.2"
echo
echo "To enter container1:"
echo "docker exec -it container1 bash"
echo
echo "To enter container2:"
echo "docker exec -it container2 bash"
echo
echo "Press Ctrl+C to cleanup and exit"

# Keep script running until Ctrl+C
while true; do
    sleep 1
done
Last edited 7 minutes ago:set nu

