
## Install Compiler
```shell
sudo apt update -y
sudo apt install -y g++ binutils libc6-dev-i386
```

## Boot Kernel From Grub

```shell
make
make install
```

After running makefile, append this section to the /boot/grub/grub.cfg file:

```
# /boot/grub/grub.cfg

### BEGIN MYKERNEL ###
menuentry 'My Operating System' {
	multiboot /boot/mykernel.bin
	boot
}
### END MYKERNEL ###
```

Reboot your computer and enter the BootLoader grub.

## Boot Kernel From VirtualBox

Install relative packages

```shell
sudo apt update -y 
sudo apt install -y virtualbox xorriso
```

Create new Virtual Machine

Type: Other
Version: Other/Unknown
RAM: 64MB
Hard Disk: Add a virutal hard disk, VDI, dynamic size, 2GB, Primary Slave

```shell
make run
```

## Boot Kernel From Parallel Desktop on Mac

The run section in Makefile should be replaced with the below section.

```shell
# Makefile
run: mykernel.iso
	prlctl delete myos
	prlctl create myos --ostype other
	prlctl set myos --device-set cdrom0 --image 'mykernel.iso' --enable --connect
	prlctl start myos
```

```shell
make run
```
