#!/bin/bash

CURDIR=$( cd "$( dirname "$0" )" && pwd ); 
echo -e "\033[0;32mClean: $CURDIR \033[0m"; 

if [ "$#" -gt "0" ] 
then
  # Clear cloned dependencies
  for name in glm 
  do
    if [ "$1" == "${name}" ] || [ "$1" == "all" ]
    then 
      echo -e "\033[0;32mClean: ${CURDIR}/dependencies/${name} \033[0m";
      rm -rf "${CURDIR}/dependencies/${name}";
    fi
  done
  # Clear build version
  for name in msvc clang gcc  
  do
    if [ "$1" == "${name}" ] || [ "$1" == "all" ]
    then 
      echo -e "\033[0;32mClean: ${CURDIR}/build/${name} \033[0m";
      rm -rf "${CURDIR}/build/${name}";
    fi    
  done
else
  rm -rf ${CURDIR}/build/ 
fi
