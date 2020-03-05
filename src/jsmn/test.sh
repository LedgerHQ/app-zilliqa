#!/usr/bin/env bash

succ=1
for jsonfile in tests/*.json
do
    outputfile=tests/`basename $jsonfile .json`.output
    output=`./jsmn.exe $jsonfile`
    if [[ $? -ne 0 ]]
    then
        echo "jsmn.exe failed for " $jsonfile
        exit 1
    fi
    # Uncomment below line to update tests.
    # ./jsmn.exe $jsonfile > $outputfile
    if [[ $output != `cat $outputfile` ]]
    then
        echo "Test $jsonfile failed"
        echo -n $output > /tmp/jsmn_output
        diff $outputfile /tmp/jsmn_output
        succ=0
    fi        
done

if [[ $succ -eq 1 ]]
then
    echo "Testsuite succeeded"
    exit 0
else
    echo "Testsuite failed"
    exit 1
fi
    
