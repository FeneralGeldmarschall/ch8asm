#pragma once

#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include "token.h"

class Compiler {
  public:
    Compiler(std::vector<Token> *tokens, char *buffer, int bufferLength);
    int compile();
    bool hadError;

  private:
    int currentAddress;
    int currentToken;
    Token *previous;
    char *buffer;
    int currentBufferPos;
    int bufferLength;

    std::vector<Token> *tokens;
    std::map<std::string, uint16_t> labelMap;
    std::map<std::string, uint16_t> variableMap;

    bool panicMode;
    void error(Token *token, const char *message, ...);

    void writeInstruction(uint16_t instruction);

    Token *advance();
    Token *peek();
    Token *peekNext();
    bool consume(TokenType type, const char *message);
    uint16_t decodeLiteral(Token *literal, uint8_t binaryLength,
                           uint8_t hexLength, const char *message);

    bool isAtEnd();
    bool check(TokenType type);
    bool checkBetween(TokenType start, TokenType end);
    bool match(TokenType type);
    bool matchBetween(TokenType start, TokenType end);

    void statement();
    void instructionStmt();
    void assignStmt(Token *identifier);

    void labelPass();

    void synchronize();
};
