#!/bin/bash

if [ $# -eq 0 ]; then
	  echo "Error: No se han pasado parametros" >&2
	  exit 1
fi

str_topic_name="$1"

if test -e ./sensors/$str_topic_name/measures.txt; then
	  rm -r ./sensors/$str_topic_name
	  sed -i /$str_topic_name/d ./sensors/list.txt
else
  	echo "Error: el sensor no está registrado"
fi