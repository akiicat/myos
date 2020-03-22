# https://wiki.osdev.org/Bare_Bones
# 這裡使用的 BootLoader 是 grub
# 
# 0x1badb002 為 bootloader header，又稱作為 magic number 
# BootLoader 在載入 kernel 的時候，會去辨識此 header 是否正確，否則會發生錯誤
# 
# BootLoader grub 會執行以下指令
#   eax = multiboot struct
#   ebx = magic number
.set MAGIC, 0x1badb002
.set FLAGS, (1<<0 | 1<<1)
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
  .long MAGIC
  .long FLAGS
  .long CHECKSUM

.section .text
.extern kernelMain
.extern callConstructors
.global loader # 進入點

# 把現有的暫存器存入 stack，然後呼叫 kernelMain 這個 function
loader:
  # 因為 C++ 在使用之前必須要先設定好 stack pointer
  # 但如果只有 kernel.cpp 所編譯的執行檔
  # 這個執行檔並不會設定 stack pointer 的位置
  # 造成 C++ 會無法在開機執行
  # 這行在設定 esp stack pointer 的起始位置
  mov $kernel_stack, %esp

  call callConstructors

  # eax: extended accumulator register
  # 裡面放的是 multiboot struct
  # 把 eax 放到 esp 的 stack 裡面，然後 esp + 4
  push %eax

  # ebx: extended base register
  # 裡面放的是 magic number
  # 把 ebx 放到 esp 的 stack 裡面，然後 esp + 4
  push %ebx
  call kernelMain # 呼叫 C++ 的 function

# 無窮迴圈
# 如果 kernelMain 的 function 執行結束，則會進入到這個地方
# 如果沒有加的話，執行完 kernelMain 之後會重新開機
_stop:
  cli
  hlt
  jmp _stop

# bss: block started by symbol 未初始化的全域變數
# 宣告一個空間當作 stack
.section .bss
.space 2*1024*1024; # 2 MiB
kernel_stack:

