GPPPARAMS = -m32 -Iinclude -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore -Wno-write-strings
ASPARAMS = --32
LDPARAMS = -melf_i386

objects = obj/loader.o \
					obj/gdt.o \
					obj/memorymanagement.o \
					obj/drivers/driver.o \
					obj/hardwarecommunication/port.o \
					obj/hardwarecommunication/interruptstubs.o \
					obj/hardwarecommunication/interrupts.o \
					obj/hardwarecommunication/pci.o \
					obj/multitasking.o \
					obj/drivers/keyboard.o \
					obj/drivers/mouse.o \
					obj/drivers/vga.o \
					obj/gui/widget.o \
					obj/gui/window.o \
					obj/gui/desktop.o \
					obj/kernel.o

obj/%.o: src/%.cpp
	echo $(@)
	mkdir -p $(@D)
	g++ $(GPPPARAMS) -o $@ -c $<

obj/%.o: src/%.s
	echo $(@)
	mkdir -p $(@D)
	as $(ASPARAMS) -o $@ $<

mykernel.bin: linker.ld $(objects)
	ld $(LDPARAMS) -T $< -o $@ $(objects)

install: mykernel.bin
	sudo cp $< /boot/mykernel.bin

mykernel.iso: mykernel.bin
	mkdir -p iso/boot/grub
	cp $< iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	echo 'menuentry "My Operating System" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/mykernel.bin' >> iso/boot/grub/grub.cfg
	echo '  boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$@ iso
	rm -rf iso

run: mykernel.iso
	(pkill VirtualBox && sleep 1) || true
	VirtualBox --startvm "My Operating System" &
	
.PHONY: clean
clean:
	rm -rf obj *.bin *.iso

