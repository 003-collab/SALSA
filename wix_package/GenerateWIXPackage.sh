#!/bin/bash

set -x
logfile=wix_log.txt
exec > $logfile 2>&1

SCRIPT_DIRECTORY=$( cd $( dirname ${BASH_SOURCE[0]} ) && pwd )
LSA_ROOT=${SCRIPT_DIRECTORY}/..

PACKAGE_ARCH=win64

# Arguments
PACKAGE_NAME=$1
BRANCH_NAME=$2
TARGET_VERSION=$3

# Constants
WIX_PATH=$WIX\bin

# Replace C: with /c
WIX_PATH=${WIX_PATH/C:/"/c"}

# Replace \ with /
WIX_PATH=${WIX_PATH//\\/"/"}

# Paths
PACKAGE_DIRECTORY=$LSA_ROOT/packages
TARGET_PATH=$PACKAGE_DIRECTORY/$PACKAGE_NAME-$TARGET_VERSION/_CPack_Packages/$PACKAGE_ARCH/WIX

TARGET_WIXPDB=$TARGET_PATH/$PACKAGE_NAME-$TARGET_VERSION-$PACKAGE_ARCH.wixpdb
TARGET_MSI=$TARGET_PATH/$PACKAGE_NAME-$TARGET_VERSION-$PACKAGE_ARCH.msi

echo "BUILD MSI"

# Build the msi
rm -rf packages/${PACKAGE_NAME}-${TARGET_VERSION}

cpack --config ${LSA_ROOT}/build/CPackConfig.cmake

cp -v packages/${PACKAGE_NAME}-${TARGET_VERSION}/_CPack_Packages/win64/WIX/${PACKAGE_NAME}-${TARGET_VERSION}-${PACKAGE_ARCH}.msi ${LSA_MSI_TARGET}/${PACKAGE_NAME}-${BRANCH_NAME}-${TARGET_VERSION}-${PACKAGE_ARCH}.msi

