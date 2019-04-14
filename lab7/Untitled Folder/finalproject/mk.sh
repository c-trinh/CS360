# mkdisk script:

touch mydisk

sudo mkfs mydisk 1440
sudo mount -o loop mydisk /mnt

(cd /mnt; sudo mkdir a b c; sudo mkdir /a/b /a/c; sudo touch f1 f2 f3 f4)

ls -l /mnt

sudo umount /mnt
