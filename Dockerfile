# Base image of ubuntu 22.04 
FROM ubuntu:22.04

# Commands to run inside of the contianer 
#iproute2 - modern commadn-line network config tools (includes ip)
#iputils - ping
#net-tools - ifconfig, netstat
# p
RUN apt-get update && apt-get install -y \
    iproute2 \
    iputils-ping \
    net-tools \
    build-essential \
    g++ \
    make \
    gdb \
    tcpdump \
    && rm -rf /var/lib/apt/lists/*

# Set up the work directory
WORKDIR /app

# Copy source file and make file into container
COPY protocol.cpp .
COPY Makefile .

# Keep container running.. What?
CMD ["tail", "-f", "/dev/null"]
