#!/bin/bash

if [ $# -eq 0 ]; then
	  echo "Error: No se han pasado parametros" >&2
	  exit 1
fi

str_topic_name="$1"

if test -e ./sensors/$str_topic_name/run.sh; then
	  echo "Error: El archivo ya existe" >&2
	  exit 1
else
  	mkdir ./sensors/$str_topic_name
	
	touch ./sensors/$str_topic_name/run.sh
	touch ./sensors/$str_topic_name/measures.txt

	echo "#!/bin/bash" >> ./sensors/$str_topic_name/run.sh
	echo "mosquitto_sub -h mqtt.eclipseprojects.io -t $str_topic_name >> measures.txt" >> ./sensors/$str_topic_name/run.sh
	
	chmod +x ./sensors/$str_topic_name/run.sh
fi



