#include "EndpointFunctions.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "Base64.h"

#define ROOT_DIR "/mnt/stuff/" // Load from config

char* getAuthCode(char* authStringPointer) {
    char authString[1024];
    strncpy(authString, authStringPointer, sizeof(authString) - 1);
    authString[sizeof(authString) - 1] = '\0';

    char username[1024] = {0};
    char password[1024] = {0};
  
    char *delimiter = strchr(authString, '.');
    if (delimiter == NULL)
        return "Invalid auth string";

    size_t username_len = delimiter - authString;
    size_t password_len = strlen(delimiter + 1);

    memcpy(username, authString, username_len);
    username[username_len] = '\0';

    memcpy(password, delimiter + 1, password_len);
    password[password_len] = '\0';

    FILE *authFile;
    authFile = fopen("auth.txt", "r");
    char line[2048];

    int i = 0;
    while(fgets(line, 2048, authFile)) {
        if (i == 0) {
            i++;
            continue;
        } // i could just aswell be a bool as it is only needed to skip the first line

        char *token = strtok(line, ".");
        if (token == NULL) {
            fclose(authFile);
            return "Invalid auth file";
        }
        
        char usernameFile[1024];
        for (int j = 0; j < 1024; j++) {
            usernameFile[j] = token[j];
        }
        if (strcmp(usernameFile, username) != 0)
            continue;

        token = strtok(NULL, ".");
        if (token == NULL) {
            fclose(authFile);
            return "Invalid auth file";
        }

        char passwordFile[1024];
        for (int j = 0; j < 1024; j++) {
            passwordFile[j] = token[j];
            if (token[j] == '\0')
                break;
        }
        if (strcmp(passwordFile, password) != 0) {
            fclose(authFile);
            return "Wrong password";
        } else {
            fclose(authFile);
            return "Has auth";
        }
    }
    fclose(authFile);
    return "Username not found in auth file";
}

char* getFiles(char* authStringPointer) {
    char authString[1024];
    strncpy(authString, authStringPointer, sizeof(authString) - 1);
    authString[sizeof(authString) - 1] = '\0';

    char username[1024] = {0};
  
    char *delimiter = strchr(authString, '.');
    if (delimiter == NULL)
        return "Invalid auth string";

    size_t username_len = delimiter - authString;
    memcpy(username, authString, username_len);
    username[username_len] = '\0';

    unsigned char *decodedUsername = NULL;
    size_t decodedLength = 0;
    size_t result = base64Decode(username, &decodedUsername, &decodedLength, 1);

    struct dirent *entry;
    char directory[1024] = ROOT_DIR;
    strcat(directory, decodedUsername);
    strcat(directory, "/");
    DIR *dp = opendir(directory);
    free(decodedUsername);

    if (dp == NULL) {
        perror("opendir");
        return "Failed to open directory";
    }

    char *files = malloc(2048);
    size_t total_entries = 0;
    while ((entry = readdir(dp))) {
        if (entry->d_name[0] != '.') {
            total_entries++;
        }
    }

    rewinddir(dp);
    size_t counter = 0;

    while ((entry = readdir(dp))) {
        if (entry->d_name[0] != '.') {
            strcat(files, entry->d_name);
            if (counter < total_entries - 1) {
                strcat(files, "\n");
            }
            counter++;
        }
    }

    closedir(dp);
    return files;
}

char* downloadFile(char* authStringPointer, char* fileName, size_t* fileSizeOut) {
    char authString[1024];
    strncpy(authString, authStringPointer, sizeof(authString) - 1);
    authString[sizeof(authString) - 1] = '\0';

    char username[1024] = {0};
  
    char *delimiter = strchr(authString, '.');
    if (delimiter == NULL)
        return "Invalid auth string";

    size_t username_len = delimiter - authString;
    memcpy(username, authString, username_len);
    username[username_len] = '\0';

    unsigned char *decodedUsername = NULL;
    size_t decodedLength = 0;
    size_t result = base64Decode(username, &decodedUsername, &decodedLength, 1);

    char directory[1024];
    snprintf(directory, sizeof(directory), "%s%s/%s", ROOT_DIR, decodedUsername, fileName);
    free(decodedUsername);

    FILE *file = fopen(directory, "rb");
    if (file == NULL) {
        perror("fopen");
        return "Failed to open file";
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize <= 0 || (size_t)fileSize > SIZE_MAX) {
        fclose(file);
        printf("Invalid file size: %ld\n", fileSize);
        fclose(file);
        *fileSizeOut = 0;
        return "Invalid file size";
    }

    char *fileContent = malloc(fileSize);
    if (fileContent == NULL) {
        fclose(file);
        return "Memory allocation failed";
    }

    fread(fileContent, 1, fileSize, file);
    fclose(file);

    *fileSizeOut = fileSize;
    return fileContent;
}