#ifndef TASBOT_EYES_FILESYSTEM_H
#define TASBOT_EYES_FILESYSTEM_H

#include <stdbool.h>

//keep
#define BASE_PATH               "./gifs/base.gif"
#define STARTUP_PATH            "./gifs/startup.gif"
#define OTHER_PATH              "./gifs/others/"
#define BLINK_PATH              "./gifs/blinks/"

extern char* pathForAnimations;
extern char* pathForBlinks;
extern char* pathForPalette;
//end keep

#define MAX_PATH_LENGTH     4096    //"The maximum combined length of both the file name and path name [on Linux]." Quote from IBM

bool getFileList(const char* _path, char* _list[]);
bool checkIfFileExist(char* _file);
bool checkIfDirectoryExist(char* _path);
int countFilesInDir(char* _path);
int countLines(const char* _path);
char* getFilePath(char* _path, char* _file);
void readFile(const char* _path, int _count, char** _out);

#endif //TASBOT_EYES_FILESYSTEM_H
