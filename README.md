


## Boot Kernel From Grub

```
# /boot/grub/grub.cfg

### BEGIN MYKERNEL ###
menuentry 'My Operating System' {
	multiboot /boot/mykernel.bin
	boot
}
### END MYKERNEL ###
```
