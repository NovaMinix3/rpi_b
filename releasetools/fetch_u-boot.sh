#!/bin/sh 
#
# Perform a checkout / update the MINIX u-boot git repo if needed
# 
# -o output dir
OUTPUT_DIR=""
GIT_VERSION=""
while getopts "o:a:b:n:d:?" c
do
        case "$c" in
        \?)
                echo "Usage: $0 -o output dir -n version " >&2
                exit 1
        	;;
        o)
                OUTPUT_DIR=$OPTARG
		;;
        a)
                GIT_SOURCE=$OPTARG
		;;
        b)
                GIT_BRANCH=$OPTARG
		;;
        n)
                GIT_VERSION=$OPTARG
		;;
        d)
                BIN_DIR=$OPTARG
		;;
	esac
done


#
# check arguments
#
if [ -z "$OUTPUT_DIR" -o -z "$GIT_VERSION" ]
then
		echo "Missing required parameters OUTPUT_DIR=$OUTPUT_DIR GIT_VERSION=$GIT_VERSION"
                echo "Usage: $0 -o output dir -n version " >&2
                exit 1
fi


#
# if the file doesn't exist it's easy , to a checkout 
#
if  [ ! -e "$OUTPUT_DIR" ]
then
	git clone $GIT_SOURCE -b $GIT_BRANCH $OUTPUT_DIR
fi

(
	cd  "$OUTPUT_DIR"

	#
	# perform an update
	#
	CURRENT_VERSION=`git rev-parse HEAD`
	if [ "$CURRENT_VERSION" !=  "$GIT_VERSION" ]
	then
		echo "Current version $CURRENT_VERSION does not match wanted $GIT_VERSION performing update and checkout"	
		git fetch -v 
		git checkout $GIT_VERSION
	fi
	make CROSS_COMPILE=arm-linux-gnueabi- rpi_b_defconfig all
	rm -rf build
	mkdir build
	mkdir ./$BIN_DIR
	cp u-boot.bin ./$BIN_DIR/u-boot.img
)
