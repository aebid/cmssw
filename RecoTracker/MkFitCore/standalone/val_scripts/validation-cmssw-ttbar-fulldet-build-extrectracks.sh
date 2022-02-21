#! /bin/bash

make -j 32 WITH_ROOT:=1

dir=/data2/slava77/samples/2021/11834.0_TTbar_14TeV+2021/
file=memoryFile.fv6.default.211008-c6b7c67.bin

NoPU=AVE_0_BX01_25ns/
PU35=AVE_35_BX01_25ns/
PU50=AVE_50_BX01_25ns/
PU70=AVE_70_BX01_25ns/

base=SKL-SP_CMSSW_TTbar

for ttbar in NoPU PU35 PU50 PU70 
do
    for bV in "BH bh" "STD std" "CE ce"
    do echo $bV | while read -r bN bO
	do
	    oBase=${base}_${ttbar}_${bN}
	    echo "${oBase}: validation [nTH:32, nVU:32]"
	    ./mkFit/mkFit --cmssw-n2seeds --cmssw-val-trkparam --input-file ${dir}/${!ttbar}/${file} --build-${bO} --num-thr 32 >& log_${oBase}_NVU32int_NTH32_cmsswval.txt
	    mv valtree.root valtree_${oBase}.root
	done
    done
done

make clean

for ttbar in NoPU PU35 PU50 PU70 
do
    tbase=${base}_${ttbar}
    for build in BH STD CE
    do
	root -b -q -l plotting/runValidation.C\(\"_${tbase}_${build}\",1\)
    done
    root -b -q -l plotting/makeValidation.C\(\"${tbase}\",\"\",1\)
done

make distclean
