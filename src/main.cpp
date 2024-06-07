#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "scanner.h"
#include "token.h"

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ch8asm [file to assemble] (outfile)\n");
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
        fprintf(stderr, "Scanning failed.\n");
        exit(65);
    }

    char compileBuffer[4096 - 512 + 1];
    Compiler compiler(&tokens, compileBuffer, 4096 - 512 + 1);
    int blen = compiler.compile();

    if (compiler.hadError) {
        fprintf(stderr, "Compiling failed.\n");
        exit(65);
    }

    const char *outfile = "out.bin";
    if (argc == 3) {
        outfile = argv[2];
    }

    FILE *outFile = fopen(outfile, "wb");
    int pos = 0;
    blen++;
    while (compileBuffer[pos] != EOF) {
        fwrite(compileBuffer + pos, 1, 1, outFile);
        pos++;
    }

    exit(0);
}
