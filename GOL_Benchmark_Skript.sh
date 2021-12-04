OMP_NUM_THREADS=8
timeStamp=$(date +"%y.%m.%d|%H:%M:%S")
log=Benchmarks/times.txt
printf "date,i,program,totaltime,cpu,memory\n" >$log
#parameters:
# 1024 x 1024
inputParameters[0]="600 1024 1024 1 1"
inputParameters[1]="600 512 1024 2 1"
inputParameters[2]="600 1024 512 1 2"
inputParameters[3]="600 256 1024 4 1"
inputParameters[4]="600 512 512 2 2"
inputParameters[5]="600 1024 256 1 4"
inputParameters[6]="600 170 1024 6 1"
inputParameters[7]="600 341 512 3 2"
inputParameters[8]="600 512 341 2 3"
inputParameters[9]="600 1024 170 1 6"
inputParameters[10]="600 128 1024 8 1"
inputParameters[11]="600 256 512 4 2"
inputParameters[12]="600 512 256 2 4"
inputParameters[13]="600 1024 128 1 8"
## 2048 x 2048
inputParameters[14]="150 2048 2048 1 1"
inputParameters[15]="150 1024 2048 2 1"
inputParameters[16]="150 2048 1024 1 2"
inputParameters[17]="150 512 2048 4 1"
inputParameters[18]="150 1024 1024 2 2"
inputParameters[19]="150 2048 512 1 4"
inputParameters[20]="150 341 2048 6 1"
inputParameters[21]="150 683 1024 3 2"
inputParameters[22]="150 1024 683 2 3"
inputParameters[23]="150 2048 341 1 6"
inputParameters[24]="150 256 2048 8 1"
inputParameters[25]="150 512 1024 4 2"
inputParameters[26]="150 1024 512 2 4"
inputParameters[27]="150 2048 256 1 8"
## 4096 x 4096
inputParameters[28]="33 4096 4096 1 1"
inputParameters[29]="33 2048 4096 2 1"
inputParameters[30]="33 4096 2048 1 2"
inputParameters[31]="33 1024 4096 4 1"
inputParameters[32]="33 2048 2048 2 2"
inputParameters[33]="33 4096 1024 1 4"
inputParameters[34]="33 683 4096 6 1"
inputParameters[35]="33 1365 2048 3 2"
inputParameters[36]="33 2048 1365 2 3"
inputParameters[37]="33 4096 683 1 6"
inputParameters[38]="33 512 4096 8 1"
inputParameters[39]="33 1024 2048 4 2"
inputParameters[40]="33 2048 1024 2 4"
inputParameters[41]="33 4096 512 1 8"

#Thread arrangements
#1: 1 1
#2: 2 1
# : 1 2
#4: 4 1
# : 2 2
# : 1 4
#6: 6 1
# : 3 2
# : 2 3
# : 1 6
# gehen nicht!?
#8: 8 1
# : 4 2
# : 2 4
# : 1 8

for j in "${inputParameters[@]}"; do
    for i in {0..4}; do
        printf "%s, %d, " $timeStamp $i >>$log
        \time -o $log -a -f " %C,  %E,  %P, %M" ./gameoflife $j
    done
    #printf "________________________________________________________________________________________________________________________\n" >>$log
done
