
// 將 string 寫到特定的記憶體位置 0xb8000
// 顯示卡則會去此位置抓取值，將文字 render 到螢幕上
//
// 以 16 個 bit 為單位組成一個文字：
//   - high byte:
//     - 4bit: front ground
//     - 4bit: back ground
//   - low byte:
//     - 8bit: char
void printf(char *str) {
	unsigned short * VideoMemory = (unsigned short *) 0xb8000;

	for (int i = 0; str[i] != '\0'; ++i) {
		VideoMemory[i] = (VideoMemory[i] & 0xFF00) | str[i];
	}
}

extern "C" void kernelMain(void *multiboot_structure, unsigned int magicnumber) {
	printf("Hello World!");

	while(1);
}
