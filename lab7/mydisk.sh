if [ $# -lt 1 ];then
   echo Usage: mkdisk NAME
   exit 1
fi

dd if=/dev/zero of=$1 bs=1024 count=1440
mke2fs -b 1024 $1 1440
mount -o loop $1 /mnt
(cd /mnt; mkdir dir1 dir2; mkdir dir1/newdir dir2/olddir; touch file1; ls -a -l)
umount /mnt
