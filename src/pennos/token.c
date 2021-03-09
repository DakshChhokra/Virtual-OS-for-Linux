#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "token.h"

char *getCommandStringFromTokens(char ***jobCommands, int commandCount) {
    // count indices and fill temporary buffer for the job desc
    unsigned int currIndex = 0;
    char buffer[4096];
    // iterate through each command
    for (int commandIdx = 0; commandIdx < commandCount; commandIdx++) {
        // for each command, iterate through each token
        int tokenIdx = 0;
        while (jobCommands[commandIdx][tokenIdx] != NULL) {
            int len = strlen(jobCommands[commandIdx][tokenIdx]);

            // add token
            for (int j = 0; j < len; j++) {
                buffer[currIndex] = jobCommands[commandIdx][tokenIdx][j];
                currIndex++;
            }

            // add whitespace
            buffer[currIndex] = ' ';
            currIndex++;

            tokenIdx++;
        }

        if (commandIdx != commandCount - 1) {
            // add room for pipe and whitespace
            buffer[currIndex] = '|';
            currIndex++;
            buffer[currIndex] = ' ';
            currIndex++;
        } else {
            // set last character to null terminator
            buffer[currIndex - 1] = '\0';
        }
    }

    // need currIndex + 1 to store null terminator
    char *res = malloc(sizeof(char) * (currIndex + 1));

    // copy over buffer to pointer
    for (int i = 0; i <= currIndex; i++) {
        res[i] = buffer[i];
    }

    return res;
}

char ***getCopyOfCommands(char ***jobCommands, int commandCount) {
    char ***res = malloc(sizeof(char**) + 1);

    if (res == NULL) {
        perror("malloc");
        return NULL;
    }

    res[1] = NULL;

    // iterate through each command
    for (int commandIdx = 0; commandIdx < commandCount; commandIdx++) {
        char **args;

        // for each command, iterate through each token
        int count = 0;
        while (jobCommands[commandIdx][count] != NULL) {
            count++;
        }

        args = malloc(sizeof(char*) * (count + 1));

        if (args == NULL) {
            free(res);
            perror("malloc");
            return NULL;
        }

        args[count] = NULL;

        res[commandIdx] = args;

        for (int i = 0; i < count; i++) {
            int len = strlen(jobCommands[commandIdx][i]);
            char *str = malloc(sizeof(char) * (len + 1));

            if (str == NULL) {
                perror("malloc");
                for (int j = 0; j < i; j++) {
                    free(args[j]);
                }
                free(args);
                free(res);
                return NULL;
            }

            strcpy(str, jobCommands[commandIdx][i]);

            args[i] = str;
        }
    }

    return res;
}