#ifndef TASBOT_EYES_FILESYSTEM_H
#define TASBOT_EYES_FILESYSTEM_H

#include <stdbool.h>
#include "utils.h"

#define BASE_PATH               "./gifs/base.gif"
#define STARTUP_PATH            "./gifs/startup.gif"
#define OTHER_PATH              "./gifs/others/"
#define BLINK_PATH              "./gifs/blinks/"

#define MAX_PATH_LENGTH     4096    //"The maximum combined length of both the file name and path name [on Linux]." Quote from IBM

extern char* pathForAnimations;
extern char* pathForBlinks;
extern char* pathForPalette;

/*
bool getFileList(const char* _path, char* _list[]);
bool checkIfFileExist(char* _file);
bool checkIfDirectoryExist(char* _path);
int countFilesInDir(char* _path);
int countLines(const char* _path);
char* getFilePath(char* _path, char* _file);
void readFile(const char* _path, int _count, char** _out);
 */

bool getFileList(const string_t* _path, string_t* _list[]);
bool checkIfFileExist(string_t* _file);
bool checkIfDirectoryExist(string_t* _path);
int countFilesInDir(string_t* _path);
int countLines(const string_t* _path);
string_t* getFilePath(string_t* _path, string_t* _file);
void readFile(const string_t* _path, int _count, string_t* _out[]);

#endif //TASBOT_EYES_FILESYSTEM_H
