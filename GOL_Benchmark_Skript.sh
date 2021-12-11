OMP_NUM_THREADS=8
timeStamp=$(date +"%y.%m.%d|%H:%M:%S")
log=Benchmarks/times.txt
printf "date,i,program,totaltime,cpu,memory\n" >$log
#parameters:
# 1024 x 1024
inputParameters[0]="2000 1024 1024"
threads[0]="1"
inputParameters[1]="2000 512 1024"
threads[1]="2"
inputParameters[2]="2000 1024 512"
threads[2]="2"
inputParameters[3]="2000 512 512"
threads[3]="4"
inputParameters[4]="2000 512 256"
threads[4]="6"
inputParameters[5]="2000 256 512"
threads[5]="6"
inputParameters[6]="2000 256 256"
threads[6]="8"
## 2048 x 2048
inputParameters[7]="500 2048 2048"
threads[7]="1"
inputParameters[8]="500 1024 2048"
threads[8]="2"
inputParameters[9]="500 2048 1024"
threads[9]="2"
inputParameters[10]="500 1024 1024"
threads[10]="4"
inputParameters[11]="500 1024 512"
threads[11]="6"
inputParameters[12]="500 512 1024"
threads[12]="6"
inputParameters[13]="500 512 512"
threads[13]="8"
## 4096 x 4096
inputParameters[14]="125 4096 4096"
threads[14]="1"
inputParameters[15]="125 2048 4096"
threads[15]="2"
inputParameters[16]="125 4096 2048"
threads[16]="2"
inputParameters[17]="125 2048 2048"
threads[17]="4"
inputParameters[18]="125 2048 1024"
threads[18]="6"
inputParameters[19]="125 1024 2048"
threads[19]="6"
inputParameters[20]="125 1024 1024"
threads[20]="8"

cnt=0
for j in "${inputParameters[@]}"; do
    for i in {0..4}; do
        printf "%s, %d, " $timeStamp $i >>$log
        \time -o $log -a -f " %C,  %E,  %P, %M" mpirun -n ${threads[$cnt]} ./gameoflife $j
    done
    cnt=$cnt+1
    #printf "________________________________________________________________________________________________________________________\n" >>$log
done
