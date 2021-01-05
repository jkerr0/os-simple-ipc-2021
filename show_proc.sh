#!/bin/bash
NAZWA=soproj
#ps -ef | grep $NAZWA$ 
ZBIOR=`ps -ef | grep $NAZWA$ | cut -d " " -f 5` 
LICZNIK=0
for i in $ZBIOR
do
      case $LICZNIK in
            "0")
                  echo -n "PM:" 
                  ;;
            "1")
                  echo -n "P1:"
                  ;;
            "2")  echo -n "P2:"
                  ;;
            "3")  echo -n "P3:"
                  ;;
      esac
      let "LICZNIK=LICZNIK+1"
      echo "$i"
done
