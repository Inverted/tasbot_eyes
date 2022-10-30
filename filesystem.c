#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "filesystem.h"
#include "stack.h"

char* pathForAnimations = OTHER_PATH;
char* pathForBlinks = BLINK_PATH;
char* pathForPalette = NULL;

/**
 * Allocate memory for path complete file path and ditch path and filename together
 * @param _path relativ or absolut path, where the file is laying
 * @param _file filename
 * @return relative or absolut path to file
 */
char* getFilePath(char* _path, char* _file) {
    char* path = malloc(sizeof(char) * (MAX_PATH_LENGTH) + 1);
    strcpy(path, _path);
    strcat(path, _file);

    return path;
}

/**
 * Check if a directory exists
 * @param _path Path of the directory, that's supposed to be checked
 * @return If the directory of the given path exists
 */
bool checkIfDirectoryExist(char* _path) {
    DIR* dir = opendir(_path);
    if (dir) {
        closedir(dir);
        return true;
    }
    fprintf(stderr, "[WARNING] Directory %s does not exist. Errno is %d\n", _path, errno);
    return false;
}

/**
 * Check if a given file exists
 * @param _file Path to the file, that should be checked
 * @return If the file for the given path exists
 */
bool checkIfFileExist(char* _file) {
    if (access(_file, F_OK) == 0) {
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

void readFile(const char* _path, int _count, char* _out[]) {
    //Check if file exists
    FILE* ptr = fopen(_path, "r");
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR] File can't be opened \n");
    }

    //Read line after line into give array
    for (int i = 0; i < _count; i++) {
        _out[i] = malloc(sizeof(unsigned int)); //todo: is this freed?
        if (!_out[i]) {
            failExit("[ERROR] readFile: Failed to allocate memory for reading file");
        }
        if (fscanf(ptr, "%s\n", _out[i]) != 1) {
            fprintf(stderr, "[ERROR] readFile: Failed to read line %d\n", i);
        }
    }

    //Closing the file
    fclose(ptr);
}

/**
 * Count the files in a given directory
 * @param _path The path of the directory, it's files should be counted
 * @return The number of files in that directory
 */
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
    fprintf(stderr, "[WARNING] countFilesInDir() called on nonexistent directory (%s). Errno is %d\n", _path, errno);
    return -1;
}

/**
 * Writes all file names of an given directory into the given _out-Array
 * @param _path The path of the directory, it file names should be written into _out
 * @param _out Pointer to output array. Results get writen into here.
 * @return If the directory was successfully open
 */
bool getFileList(const char* _path, char* _out[]) {
    DIR* d = opendir(_path);
    if (d) {
        int counter = 0;
        struct dirent* dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                _out[counter] = dir->d_name;
                counter++;
            }
        }
        closedir(d);
        return true;
    }
    fprintf(stderr, "[WARNING] getFileList() called on nonexistent directory %s. Errno is %d\n", _path, errno);
    return false;
}

/**
 * Count the lines of a file
 * @param _path The file whose lines should be count
 * @return The number of lines
 */
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
}
