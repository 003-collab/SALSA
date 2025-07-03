#!/bin/bash
# build our LaTeX document

pushd . > /dev/null # save current directory to our stack



SCRIPT_PATH=${BASH_SOURCE[0]/C:/\/c}
SCRIPT_PATH=${SCRIPT_PATH//\\/\/}
SCRIPT_DIRECTORY=$(cd $( dirname ${SCRIPT_PATH}) && pwd)

if [ $# -eq 0 ]
    then
        SALSA_BUILD_DIRECTORY=$SCRIPT_DIRECTORY
    else
        SALSA_BUILD_DIRECTORY=$1
fi




cd $SCRIPT_DIRECTORY

LaTeXFile=SalsaUserManual
echo -e "\nAttempting to compile $LaTeXFile.tex...\n"
sleep 1
pdflatex $LaTeXFile
bibtex $LaTeXFile
pdflatex $LaTeXFile
pdflatex $LaTeXFile | tee pdflog.txt
# clean up
rm *.aux
rm *.log
rm *.toc
rm *.bbl
rm *.blg
rm *.out
if (( `grep -c -i "Warning" pdflog.txt` > 0 )); then
  echo -e "\n###############################################"
  echo -e "### Warnings from last pdflatex run follow: ###"
  echo -e "###############################################\n"
  grep -A 1 -i "Warning" pdflog.txt
fi

if (( `grep -c -i "LaTeX Error:" pdflog.txt` > 0 )); then
    rm ${LaTeXFile}.pdf
    exit 1
fi
rm pdflog.txt

# I believe the following is a remnant of an earlier build implementation wherein
# we called this script from our cmake build process.  Now that we no longer do that,
# a user who runs this Compile.sh script probably does not intend for the pdf file
# to be copied into some other directory.
# mv ${LaTeXFile}.pdf $SALSA_BUILD_DIRECTORY

popd > /dev/null 

