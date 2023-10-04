#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "scanner.h"
#include "token.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: chesm [path]\n");
        exit(64);
    }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\"", argv[1]);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", argv[1]);
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", argv[1]);
        exit(74);
    }
    buffer[bytesRead] = '\0';
    fclose(file);

    std::vector<Token> tokens;
    Scanner scanner(buffer);
    scanner.scan(&tokens);

    if (scanner.hadError) {
        fprintf(stderr, "Scanning failed.");
        exit(65);
    }

    char compileBuffer[4096 - 512 + 1];
    Compiler compiler(&tokens, "out.bin", compileBuffer, 4096 - 512 + 1);
    compiler.compile();

    if (compiler.hadError) {
        fprintf(stderr, "Compiling failed.");
        exit(65);
    }

    FILE *outFile = fopen("out.ch8", "wb");
    int pos = 0;
    while (buffer[pos] != '\0') {
        fwrite(&buffer[pos], 1, 1, outFile);
        pos++;
    }

    exit(0);
}