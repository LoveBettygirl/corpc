#!/bin/bash

FILE_NAME=$1
PROJECT_NAME=$(basename ${FILE_NAME})
CURRENT_PATH=$(cd $(dirname $0); pwd)
PROJECT_ROOT_PATH=$(cd $(dirname $0); cd ..; pwd)
PROJECT_BIN_FILE="${CURRENT_PATH}"/"bin/${PROJECT_NAME}"
PROJECT_CONF_FILE="./conf/${PROJECT_NAME}.xml"


echo "Run corpc project, name: ${PROJECT_NAME}, path: ${PROJECT_BIN_FILE}"

if [ -z "$1" ]
then
    echo "Please input execuable binary file!"
fi

# check bin file exist
if [ ! -e ${PROJECT_BIN_FILE} ]
then
    echo "Run corpc eror, file: ${PROJECT_BIN_FILE} not exist, please check file"
    exit -1
fi

# check config xml file exist
if [ ! -e ${PROJECT_CONF_FILE} ]
then
    echo "Run corpc error, file: ${PROJECT_CONF_FILE} not exist, please check config file"
    exit -1
fi

# check bin file execute privilege
if [ ! -x ${PROJECT_BIN_FILE} ]
then
    echo "chmod +x : ${PROJECT_BIN_FILE}"
    chmod +x ${PROJECT_BIN_FILE}
fi

sh shutdown.sh ${PROJECT_NAME}
nohup ./${PROJECT_NAME} ${PROJECT_CONF_FILE} & > ${PROJECT_ROOT_PATH}/log/${PROJECT_NAME}.nohup_log
echo "Start corpc server ${PROJECT_CONF_FILE} succ"
