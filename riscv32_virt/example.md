## 用FAT映像传递文件<a name="sectionfatfs"></a>
---

利用riscv32 virt的第二个CFI flash设备，可以加载FAT格式的映像盘。因为FAT映像制作、挂载、存储文件均比较简单，可由此在宿主机和虚拟机间方便地传递文件。

1. 准备FAT映像
```

dd if=/dev/zero of=fat.img bs=32M count=1
sudo losetup /dev/loop0 fat.img
sudo fdisk /dev/loop0    # 磁盘分区选择MBR格式, FAT16或FAT32
sudo losetup -o 1048576 /dev/loop1 /dev/loop0    # 这里用第一个主分区示例
sudo mkfs.fat /dev/loop1
```

2. 在虚拟机中挂载
```
qemu-system-riscv32 ...(正常运行参数) \
                -drive if=pflash,file=fat.img,format=raw,index=1

OHOS # mount /dev/cfiblk some_dir fat  
```

**注意**：device必须指定index=1参数，在riscv32 virt中，挂载目录必须是以下4个目录之一：/system、/inner、/update、/user

3. 在宿主机中挂载
```
sudo losetup /dev/loop0 fat.img
sudo losetup -o 1048576 /dev/loop1 /dev/loop0
sudo mount /dev/loop1 some_dir
```

4. 缷载
```
sudo umount some_dir

sudo losetup -d /dev/loop1    # 宿主机时
sudo losetup -d /dev/loop0    # 宿主机时
```


