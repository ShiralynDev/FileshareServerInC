#include <stddef.h>

char* getAuthCode(char* authStringPointer);
char* getFiles(char* authStringPointer);
char* downloadFile(char* authStringPointer, char* fileName, size_t* fileSizeOut);