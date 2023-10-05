#pragma once

#include <vector>

#include "token.h"

class Scanner {
  public:
    bool hadError;
    Scanner(const char *source);
    void scan(std::vector<Token> *vector);

  private:
    const char *start;
    const char *current;
    int line;
    bool panicMode;

    void error(int line, char c, const char *message);

    char previous();
    char advance();
    char peek();
    char peekNext();
    bool isAtEnd();
    Token newToken(TokenType type);
    void skipWhitespace();
    void comment();

    Token literal();
    Token identifier(char c);

    TokenType checkInstruction(int start, int length, const char *rest,
                               TokenType type);
    TokenType identifierOrInstruction();
};
