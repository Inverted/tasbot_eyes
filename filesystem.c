#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "filesystem.h"
#include "stack.h"

//keep
char* pathForAnimations = OTHER_PATH;
char* pathForBlinks = BLINK_PATH;
char* pathForPalette = NULL;
//end keep

//todo: migrate modified versions

void createNewStack() {
    string_t s;
    initstr(&s, OTHER_PATH);
    fillStack(&s);
}

void fillStack(string_t* _sourceFolder) {
    int fileCount = countFilesInDir(_sourceFolder); //get file count

    //if (fileCount != -1) {
    if (fileCount > 0) {

        //Fill array with all possible indices
        int index[fileCount];
        for (int i = 0; i < fileCount; ++i) {
            index[i] = i;
        }

        //Shuffle said array
        shuffle(index, fileCount);

        //char* list[fileCount];
        string_t* list[fileCount];
        getFileList(_sourceFolder, list); //get list of files

        for (int i = 0; i < fileCount; ++i) {
            addToStack(list[index[i]]);
        }
    }
}

//todo: use this also in networking
/**
 * Add a given path to the animation stack
 * @param _path Path to an animation that's allocated in memory already!
 */
bool addToStack(string_t* _path) {
    if (push(_path)) {
        string_t* speek = (string_t*) peek();
        printf("[INFO] Successfully added (%s) to animation stack\n",
               speek->buffer); //Using peeked value to reinsure it got added properly
        return true;
    } //else
    printf("[WARNING] Failed to add (%s) to animation stack. Stack most likely full\n", _path->buffer);
    return false;

    //todo: consume and free
}

/**
 * Allocate memory for path complete file path and ditch path and filename together
 * @param _path relativ or absolut path, where the file is laying
 * @param _file filename
 * @return relative or absolut path to file
 */
string_t* getFilePath(string_t* _path, string_t* _file) {
    //string_t* path = malloc(sizeof(char) * (MAX_PATH_LENGTH));
    string_t* path = alloccstr(MAX_PATH_LENGTH);

    strcpy(path->buffer, _path->buffer);
    strcat(path->buffer, _file->buffer);

    return path;
}

/**
 * Check if a directory exists
 * @param _path Path of the directory, that's supposed to be checked
 * @return If the directory of the given path exists
 */
bool checkIfDirectoryExist(string_t* _path) {
    DIR* dir = opendir(_path->buffer);
    if (dir) {
        closedir(dir);
        return true;
    }
    fprintf(stderr, "[WARNING] Directory %s does not exist. Errno is %d\n", _path->buffer, errno);
    return false;
}

/**
 * Check if a given file exists
 * @param _file Path to the file, that should be checked
 * @return If the file for the given path exists
 */
bool checkIfFileExist(string_t* _file) {
    if (access(_file->buffer, F_OK) == 0) {
        return true;
    }
    return false;
}

/**
 * Read lines of a given file into a given array
 * @param _path The file, that should be parsed into the array
 * @param _count The count of lines, the file has
 * @param _out The array, the lines should be read into
 */

//todo: make sure readPalette() inits out array
void readFile(const string_t* _path, int _count, string_t* _out[]) {
    //Check if file exists
    FILE* ptr = fopen(_path->buffer, "r");
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR] File can't be opened \n");
    }

    //Read line after line into give array
    for (int i = 0; i < _count; i++) {

        char val[6];
        if (fscanf(ptr, "%s\n", val) != 1) {
            fprintf(stderr, "[ERROR] readFile: Failed to read line %d\n", i);
        }

        _out[i] = allocstr(val);
    }

    //Closing the file
    fclose(ptr);
}

/**
 * Count the files in a given directory
 * @param _path The path of the directory, it's files should be counted
 * @return The number of files in that directory
 */
int countFilesInDir(string_t* _path) {
    DIR* d = opendir(_path->buffer);
    if (d) {
        int counter = 0;
        struct dirent* dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                counter++;
            }
        }
        closedir(d);
        return counter;
    }
    fprintf(stderr, "[WARNING] countFilesInDir() called on nonexistent directory (%s). Errno is %d\n", _path->buffer,
            errno);
    return -1;
}

/**
 * Writes all file names of an given directory into the given _list-Array
 * @param _path The path of the directory, it file names should be written into _list
 * @param _list Pointer to output array. Results get writen into here.
 * @return If the directory was successfully open
 */
bool getFileList(const string_t* _path, string_t* _list[]) {
    DIR* d = opendir(_path->buffer);
    if (d) {
        int counter = 0;
        struct dirent* dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                string_t* s = allocstr(dir->d_name);
                _list[counter] = s;
                counter++;
            }
        }
        closedir(d);
        return true;
    }
    fprintf(stderr, "[WARNING] getFileList() called on nonexistent directory (%s). Errno is %d\n", _path->buffer,
            errno);
    return false;
}

/**
 * Count the lines of a file
 * @param _path The file whose lines should be count
 * @return The number of lines
 */
int countLines(const string_t* _path) {
    //Check if file exists
    FILE* ptr = fopen(_path->buffer, "r");
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR] File can't be opened \n");
    }

    //Read every character of the file and count new lines/line feeds
    int newLines = 0;
    int ch;
    do {
        ch = fgetc(ptr);
        if (ch == '\n') {
            newLines++;
        }
    } while (ch != EOF);

    //Closing the file
    fclose(ptr);

    //Last line doesn't have a line feed, so add one
    return newLines + 1;
}

/**
 * Allocate memory for path complete file path and ditch path and filename together
 * @param _path relativ or absolut path, where the file is laying
 * @param _file filename
 * @return relative or absolut path to file
 *//*

char* getFilePath(char* _path, char* _file) {
    char* path = malloc(sizeof(char) * (MAX_PATH_LENGTH));
    strcpy(path, _path);
    strcat(path, _file);

    return path;
}

*/
/**
 * Check if a directory exists
 * @param _path Path of the directory, that's supposed to be checked
 * @return If the directory of the given path exists
 *//*

bool checkIfDirectoryExist(char* _path) {
    DIR* dir = opendir(_path);
    if (dir) {
        closedir(dir);
        return true;
    }
    fprintf(stderr, "[WARNING] Directory %s does not exist. Errno is %d\n", _path, errno);
    return false;
}

*/
/**
 * Check if a given file exists
 * @param _file Path to the file, that should be checked
 * @return If the file for the given path exists
 *//*

bool checkIfFileExist(char* _file) {
    if (access(_file, F_OK) == 0) {
        return true;
    }
    return false;
}

*/
/**
 * Read lines of a given file into a given array
 * @param _path The file, that should be parsed into the array
 * @param _count The count of lines, the file has
 * @param _out The array, the lines should be read into
 *//*

void readFile(const char* _path, int _count, char** _out) {
    //Check if file exists
    FILE* ptr = fopen(_path, "r");
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR] File can't be opened \n");
    }

    //Read line after line into give array
    for (int i = 0; i < _count; i++) {
        _out[i] = malloc(sizeof(unsigned int)); //todo: inspect
        if (!_out[i]) {
            fprintf(stderr, "[ERROR] readFile: Failed to allocate memory for reading file");
            clearLEDs();
            exit(EXIT_FAILURE);
        }
        if (fscanf(ptr, "%s\n", _out[i]) != 1) {
            fprintf(stderr, "[ERROR] readFile: Failed to read line %d\n", i);
        }
    }

    //Closing the file
    fclose(ptr);
}

*/
/**
 * Count the files in a given directory
 * @param _path The path of the directory, it's files should be counted
 * @return The number of files in that directory
 *//*

int countFilesInDir(char* _path) {
    DIR* d = opendir(_path);
    if (d) {
        int counter = 0;
        struct dirent* dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                counter++;
            }
        }
        closedir(d);
        return counter;
    }
    fprintf(stderr, "[WARNING] countFilesInDir() called on nonexistent directory %s. Errno is %d\n", _path, errno);
    return -1;
}

*/
/**
 * Writes all file names of an given directory into the given _list-Array
 * @param _path The path of the directory, it file names should be written into _list
 * @param _list Pointer to output array. Results get writen into here.
 * @return If the directory was successfully open
 *//*

bool getFileList(const char* _path, char* _list[]) {
    DIR* d = opendir(_path);
    if (d) {
        int counter = 0;
        struct dirent* dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                _list[counter] = dir->d_name;
                counter++;
            }
        }
        closedir(d);
        return true;
    }
    fprintf(stderr, "[WARNING] getFileList() called on nonexistent directory %s. Errno is %d\n", _path, errno);
    return false;
}

*/
/**
 * Count the lines of a file
 * @param _path The file whose lines should be count
 * @return The number of lines
 *//*

int countLines(const char* _path) {
    //Check if file exists
    FILE* ptr = fopen(_path, "r");
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR] File can't be opened \n");
    }

    //Read every character of the file and count new lines/line feeds
    int newLines = 0;
    int ch;
    do {
        ch = fgetc(ptr);
        if (ch == '\n') {
            newLines++;
        }
    } while (ch != EOF);

    //Closing the file
    fclose(ptr);

    //Last line doesn't have a line feed, so add one
    return newLines + 1;
}*/
