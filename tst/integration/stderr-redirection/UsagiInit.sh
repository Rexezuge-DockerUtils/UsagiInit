./resources/write_stderr.sh redirect-merged 2>&1
./resources/write_stderr.sh redirect-hidden 2>/dev/null
echo redirect-after-hidden
./resources/write_stderr.sh redirect-file 2> stderr-output.txt
cat stderr-output.txt
rm stderr-output.txt
exit
