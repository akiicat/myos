# BootLoader 在載入 kernel 的時候，需要辨識 magic header 是否正確
# 否則使用 gruB 載入的時候，會發生錯誤
.set MAGIC, 0x1badb002
.set FLAGS, (1<<0 | 1<< 1)
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
	.long MAGIC
	.long FLAGS
	.long CHECKSUM

.section .text
.extern kernelMain
.global loader

# 把現有的暫存器存入 stack，然後呼叫 kernelMain 這個 function
loader:
  # 因為 C++ 在使用之前必須要先設定好 stack pointer
  # 但如果只有 kernel.cpp 所編譯的執行檔
  # 這個執行檔並不會設定 stack pointer 的位置
  # 造成 C++ 會無法在開機執行
  # 這行在設定 esp stack pointer 的起始位置
	mov $kernel_stack, %esp   
	push %eax # 把 eax extended accumulator register 放到 stack 裡面，然後 esp + 4
	push %ebx # 把 eax extended base register 放到 stack 裡面，然後 esp + 4
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

