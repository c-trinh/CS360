# mkdisk script:

#touch mydisk

#sudo mkfs mydisk 1440
#sudo mount -o loop mydisk /mnt

#(cd /mnt;sudo mkdir a b c; sudo mkdir /a/b /a/c; sudotouch f1 f2 f3 f4)

#ls -l /mnt

#sudo umount /mnt

#cc -m32 source.c


cp ~/root/mydisk /home/ann/root/CPTS360/"WORKING12"
#mv  mydisk
cc -w -m32 source.c
./a.out
