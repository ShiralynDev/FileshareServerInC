#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define HTTPSERVER_IMPL
#include "httpserver.h"

#include "EndpointFunctions.h"


void handle_request(struct http_request_s* request) {
    char* contentType = "text/plain";
    char responseBody[1024] = "Hello, World!"; 
    http_string_t authHeader = http_request_header(request, "Authorization");
    if (authHeader.buf == NULL) {
        strcpy(responseBody, "Missing Authorization header");
        goto RESPONSE;
    }
    char cleanAuthHeader[1024];
    size_t len = strlen(authHeader.buf);
    while (len > 0 && isspace(authHeader.buf[len-1])) {
        len--;
    }
    strcpy(cleanAuthHeader, authHeader.buf);
    cleanAuthHeader[len] = '\0';

    char* authResponse = getAuthCode(cleanAuthHeader);
    if (strcmp(authResponse, "Has auth")) {
        strcpy(responseBody, authResponse);
        goto RESPONSE;
    }

    char path[1024];
    http_string_t requestTarget = http_request_target(request);
    char *delimiter = strchr(requestTarget.buf, ' ');
    size_t username_len = delimiter - requestTarget.buf;
    memcpy(path, requestTarget.buf, username_len);
    path[username_len] = '\0';
    size_t fileSize = 0;
    
    if (strcmp(path, "/getFiles/") == 0) {
        char* files = getFiles(cleanAuthHeader);
        strcpy(responseBody, files);
    } else if (strncmp(path, "/downloadFile/", strlen("/downloadFile/")) == 0) {
        char *filename = path + strlen("/downloadFile/");
        if (*filename == '\0') {
            strcpy(responseBody, "Filename not provided");
        } else {
            size_t fileSize;
            char* filedata = downloadFile(cleanAuthHeader, filename, &fileSize);
            if (filedata == NULL || strcmp(filedata, "Failed to open file") == 0 || strcmp(filedata, "Memory allocation failed") == 0) {
                strcpy(responseBody, filedata ? filedata : "Unknown error");
            } else {
                // Determine the Content-Type based on the file extension
                const char* contentType = "application/octet-stream";
                if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) {
                    contentType = "image/jpeg";
                } else if (strstr(filename, ".png")) {
                    contentType = "image/png";
                } else if (strstr(filename, ".gif")) {
                    contentType = "image/gif";
                } else if (strstr(filename, ".html")) {
                    contentType = "text/html";
                } else if (strstr(filename, ".txt")) {
                    contentType = "text/plain";
                } else if (strstr(filename, ".mp4")) {
                    contentType = "video/mp4";
                }

                struct http_response_s* response = http_response_init();
                http_response_status(response, 200);
                http_response_header(response, "Content-Type", contentType);
                http_response_body(response, filedata, fileSize); // Send binary data
                http_respond(request, response);
                free(filedata); // Free allocated memory for file data
                return; // Exit after responding
            }
        }
    } else {
        strcpy(responseBody, "Unknown endpoint");
    }

    RESPONSE:
    int responseBodySize = strlen(responseBody);
    responseBody[responseBodySize + 1] = '\0';

    struct http_response_s* response = http_response_init();
    http_response_status(response, 200);
    http_response_header(response, "Access-Control-Allow-Origin", "*");
    http_response_header(response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    http_response_header(response, "Content-Type", "text/plain");
    http_response_header(response, "Access-Control-Allow-Headers", contentType);
    if (fileSize > 0) {
        http_response_body(response, responseBody, fileSize);
    } else {
        http_response_body(response, responseBody, strlen(responseBody));
    }
    http_respond(request, response);
}

int main() {
    struct http_server_s* server = http_server_init(8080, handle_request);
    http_server_listen(server);
}