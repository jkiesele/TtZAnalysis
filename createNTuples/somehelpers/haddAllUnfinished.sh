#!/bin/zsh
mkdir -p haddlog
for f in naf_*; do; 
    if [[ "${f}" == *"run"* ]]; then  
	echo "not using ${f} (is data)";
    else; 
	echo "${f} is going to be hadded" ; 
	haddUnfinished.sh $f > haddlog/$f.hadd.out &; 
    fi; 
    echo 
done