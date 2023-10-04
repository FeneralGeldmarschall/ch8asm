#pragma once

#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include "token.h"

// typedef enum {
//     INST_CLS,
//     INST_RET,
//     INST_JP,
//     INST_CALL,
//     INST_SE,
//     INST_SNE,
//     INST_LD,
//     INST_ADD,
//     INST_OR,
//     INST_XOR,
//     INST_SUB,
//     INST_SHR,
//     INST_SUBN,
//     INST_SHL,
//     INST_RND,
//     INST_DRW,
//     INST_SKP,
//     INST_SKNP,
//     INST_UNKNOWN,
// } InstructionType;

class Compiler {
  public:
    Compiler(std::vector<Token> *tokens, const char *outfile, char *buffer,
             int bufferLength);
    void compile();
    bool hadError;

  private:
    int currentAddress;
    int currentToken;
    Token *previous;
    // const char *outfile;
    FILE *outfile;
    char *buffer;
    int currentBufferPos;
    int bufferLength;

    std::vector<Token> *tokens;
    std::map<std::string, uint16_t> labelMap;
    std::map<std::string, uint16_t> variableMap;

    bool panicMode;
    void errorAtPrevious(const char *message);
    void error(Token *token, const char *message);

    void writeInstruction(uint16_t instruction);

    Token *advance();
    Token *peek();
    Token *peekNext();
    bool consume(TokenType type, const char *message);
    bool decodeLiteral(Token *literal, uint8_t binaryLength, uint8_t hexLength,
                       const char *message);

    // Token *previous();
    bool isAtEnd();
    bool match(TokenType type);
    bool matchBetween(TokenType start, TokenType end);

    void statement();
    void instructionStmt();
    void labelStmt();
    void assignStmt();

    void labelPass();

    void synchronize();
};