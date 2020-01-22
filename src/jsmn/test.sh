#!/usr/bin/env bash

succ=1
for jsonfile in *.json
do
    outputfile=`basename $jsonfile .json`.output
    output=`./jsmn.exe $jsonfile`
    if [[ $output != `cat $outputfile` ]]
    then
        echo "Test $jsonfile failed"
        echo -n $output > /tmp/jsmn_output
        diff $outputfile /tmp/jsmn_output
        succ=0
    fi        
done

if [[ $succ ]]
then
    echo "Testsuite succeeded"
    exit 0
else
    echo "Testsuite failed"
    exit 1
fi
    
