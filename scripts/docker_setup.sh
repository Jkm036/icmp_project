#!/bin/bash
DOCKER_FILE_DIR="../"
#TEST_REPO="hello-world"
LOG_TAG="setup: "
# Right now this will just pull images for me to test docker_cleanup.sh
# Plann on adding more to this later
docker build -t node-template "${DOCKER_FILE_DIR}"
#docker pull ${TEST_REPO}
echo "${LOG_TAG} done.." 
docker images
