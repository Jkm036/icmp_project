#!/bin/bash
ALL_RE="[aA][Ll][Ll]"
LOG_TAG="cleanup script: "
cleaning_all=0
clean_images(){
	# Eveything passed into this function should be kept
	kept_images=$@
	if ! [ ${cleaning_all} ]; then 
		echo "$LOG_TAG Images to protect from clean up... $kept_images"
	fi
	for image in $(docker images --format "{{.Repository}}:{{.ID}}"); do
		# Start off with the assumption that we are deleting this image
		delete=1

		# Removing suffix ('%') and prefix ('#')
		repo=${image%:*}
		id=${image#*:}

		# Iterate through our passed in argument_list and check for mathes
		for kept in ${kept_images}; do
			if [ "${repo}" == "${kept}" ]; then
				delete=0
			fi
		done

		# Check if this is a safe repo, if not execute it ... >:)
		if [ ${delete} -ne 0 ]; then
			echo "$LOG_TAG Deleting ${repo}..."
			echo " delete is equal to ${delete}"
			docker rmi ${id} --force
		else
			echo "$LOG_TAG Keeping ${repo}..."
		fi


	done

}

# Does all exist in argument list
all_exists(){
	agrument_list=$@
	for tag in "${argument_list}"; do 
		# checking if all command is being used
		echo "${tag}" | grep -qE ${ALL_RE}
		if [ $? -eq 0 ];then 
			cleaning_all=1
			return 1
		fi
	done
	return 0
}
# Checking number of arguments
if [ $# -lt 1 ]; then 
	# Check that we have SOME arguments
	echo "$LOG_TAG Need to pass in image to keep! use 'all' if you want to clean all"
	exit 1
# Here I should probably check if Allis passed in
elif [ $(all_exists $@) ]; then 


	# If using All you can't specify anything else 
	# Using this as a guard to protect against accidents
	if [ $# -gt 1 ]; then
		echo "$LOG_TAG Must use all with NO other arguments.."
		exit 1
	fi
	# Run the commadn in question 	
	echo "$LOG_TAG Deleting all images stored locally ..."
	docker rmi $(docker images -q) --force
	if [ $? -eq 0 ]; then 
		echo "$LOG_TAG Removed all images... check 'docker images' to make sure. :)"
		exit 0
	else
		echo "$LOG_TAG Something went wrong with removing images..."
		exit 1
	fi 
else
	# remove everything except arguments
	clean_images $@
fi
