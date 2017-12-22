#include "DataEEPROM.h"

int ReadEE(int Page, int Offset, int* DataOut, int Size) {
	(void)Size;
	if(Page==0x7F && Offset==0xFFFE) {
		*DataOut = 0xFFF7; // PO8030 with no rotation like with e-puck1.x.
	} else {
		*DataOut = 0; // No eeprom available.
	}
	return 0;
}

int EraseEE(int Page, int Offset, int Size) {
	(void)Page;
	(void)Offset;
	(void)Size;
	return 0; // No eeprom available.
}

int WriteEE(int* DataIn, int Page, int Offset, int Size) {
	(void)DataIn;
	(void)Page;
	(void)Offset;
	(void)Size;
	return 0;  // No eeprom available.
}


