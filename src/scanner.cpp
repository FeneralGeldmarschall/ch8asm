#include <cstring>
#include <stdio.h>

#include "common.h"
#include "scanner.h"
#include "token.h"

void Scanner::error(int line, char c, const char *message) {
    if (!panicMode) {
        fprintf(stderr, "[line %d] '%c' %s\n", line, c, message);
        panicMode = true;
    }
    hadError = true;
}

Scanner::Scanner(const char *source) {
    this->start = source;
    this->hadError = false;
    this->current = source;
    this->line = 1;
}

char Scanner::previous() {
    // TODO - this can cause a segfault if current points at the start of the
    //        string
    return this->current[-1];
}

char Scanner::advance() {
    this->current++;
    return this->current[-1];
}

char Scanner::peek() { return this->current[0]; }

char Scanner::peekNext() { return this->current[1]; }

bool Scanner::isAtEnd() { return this->current[0] == '\0'; }

Token Scanner::newToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = this->start;
    token.length = this->current - this->start;
    token.line = this->line;
    return token;
}

Token Scanner::literal() {
    if (*this->start == '%') {
        while (isBinary(peek()))
            advance();
    } else if (*this->start == '$') {
        while (isHex(peek()))
            advance();
    }

    return newToken(TOKEN_LITERAL);
}

void Scanner::skipWhitespace() {
    while (true) {
        char c = peek();

        if (c == ' ' || c == '\t') {
            advance();
        } else {
            return;
        }
    }
}

void Scanner::comment() {
    while (!isAtEnd()) {
        char c = advance();
        if (c == '\n') {
            line++;
            return;
        }
    }
}

TokenType Scanner::checkInstruction(int start, int length, const char *rest,
                                    TokenType type) {
    if (this->current - this->start == start + length &&
        memcmp(this->start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

TokenType Scanner::identifierOrInstruction() {
    int tokenLength = this->current - this->start;
    if (tokenLength > 4)
        return TOKEN_IDENTIFIER;

    switch (this->start[0]) {
        case 'A': {
            switch (this->start[1]) {
                case 'D': {
                    return checkInstruction(2, 1, "D", TOKEN_INST_ADD);
                } break;
                case 'N': {
                    return checkInstruction(2, 1, "D", TOKEN_INST_AND);
                } break;
            }
        } break;
        case 'B': {
            return checkInstruction(1, 2, "CD", TOKEN_INST_BCD);
        } break;
        case 'C': {
            switch (this->start[1]) {
                case 'L': {
                    return checkInstruction(2, 1, "S", TOKEN_INST_CLS);
                } break;
                case 'A': {
                    return checkInstruction(2, 2, "LL", TOKEN_INST_CALL);
                } break;
            }
        } break;
        case 'D': {
            return checkInstruction(1, 2, "RW", TOKEN_INST_DRW);
        } break;
        case 'F': {
            return checkInstruction(1, 2, "NT", TOKEN_INST_FNT);
        } break;
        case 'G': {
            return checkInstruction(1, 2, "DT", TOKEN_INST_GDT);
        } break;
        case 'J': {
            if (tokenLength == 2) {
                return checkInstruction(1, 1, "P", TOKEN_INST_JP);
            } else {
                return checkInstruction(1, 2, "PO", TOKEN_INST_JPO);
            }
        } break;
        case 'L': {
            if (tokenLength == 2) {
                return checkInstruction(1, 1, "D", TOKEN_INST_LD);
            } else {
                return checkInstruction(1, 2, "DV", TOKEN_INST_LDV);
            }
        } break;
        case 'O': {
            return checkInstruction(1, 1, "R", TOKEN_INST_OR);
        } break;
        case 'R': {
            switch (this->start[1]) {
                case 'E': {
                    return checkInstruction(2, 1, "T", TOKEN_INST_RET);
                } break;
                case 'N': {
                    return checkInstruction(2, 1, "D", TOKEN_INST_RND);
                }
            }
        } break;
        case 'S': {
            switch (this->start[1]) {
                case 'U': {
                    if (tokenLength == 3) {
                        return checkInstruction(2, 1, "B", TOKEN_INST_SUB);
                    } else {
                        return checkInstruction(2, 2, "BN", TOKEN_INST_SUBN);
                    }
                } break;
                case 'K': {
                    if (tokenLength == 3) {
                        return checkInstruction(2, 1, "P", TOKEN_INST_SKP);
                    } else {
                        return checkInstruction(2, 2, "NP", TOKEN_INST_SKNP);
                    }
                } break;
                case 'H': {
                    if (this->start[1] == 'H') {
                        if (tokenLength == 3) {
                            if (this->start[2] == 'L') {
                                return TOKEN_INST_SHL;
                            } else if (this->start[2] == 'R') {
                                return TOKEN_INST_SHR;
                            }
                        }
                    }
                    return TOKEN_IDENTIFIER;
                } break;
                case 'E': {
                    return checkInstruction(0, 2, "SE", TOKEN_INST_SE);
                } break;
                case 'N': {
                    return checkInstruction(2, 1, "E", TOKEN_INST_SNE);
                } break;
                case 'D': {
                    return checkInstruction(2, 1, "T", TOKEN_INST_SDT);
                } break;
                case 'S': {
                    return checkInstruction(2, 1, "T", TOKEN_INST_SST);
                } break;
                case 'T': {
                    return checkInstruction(2, 1, "V", TOKEN_INST_STV);
                } break;
            }
        } break;
        case 'W': {
            return checkInstruction(1, 2, "KP", TOKEN_INST_WKP);
        } break;
        case 'X': {
            return checkInstruction(1, 2, "OR", TOKEN_INST_XOR);
        } break;
    }

    return TOKEN_IDENTIFIER;
}

Token Scanner::identifier(char c) {
    // Handle registers
    if (c == 'I' && !isAlphaNumeric(peek())) {
        return newToken(TOKEN_I_REGISTER);
    }
    if ((c == 'v' || c == 'V') && isHex(peek()) && !isAlpha(peekNext())) {
        advance();
        return newToken(TOKEN_V_REGISTER);
    }

    // Handle the rest
    while (isAlpha(peek())) {
        advance();
    }
    return newToken(identifierOrInstruction());
}

void Scanner::scan(std::vector<Token> *vector) {
    while (!isAtEnd()) {

        skipWhitespace();
        this->start = this->current;
        char c = advance();

        if (isAlpha(c)) {
            vector->push_back(identifier(c));
            continue;
        }

        switch (c) {
            case ':': {
                vector->push_back(newToken(TOKEN_COLON));
            } break;
            case '.': {
                vector->push_back(newToken(TOKEN_DOT));
            } break;
            case ',': {
                vector->push_back(newToken(TOKEN_COMMA));
            } break;
            case '\n': {
                vector->push_back(newToken(TOKEN_NEWLINE));
                this->line++;
                this->panicMode = false;
            } break;
            case '=': {
                vector->push_back(newToken(TOKEN_EQUAL));
            } break;
            case '$': {
                vector->push_back(literal());
            } break;
            case '%': {
                vector->push_back(literal());
            } break;
            case ';': {
                comment();
            } break;
            default: {
                error(this->line, c, "Unexpected character.");
            } break;
        }
    }
}