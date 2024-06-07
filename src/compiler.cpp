#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include "common.h"
#include "compiler.h"
#include "token.h"

Compiler::Compiler(std::vector<Token> *tokens, char *buffer, int bufferLength) {
    this->tokens = tokens;
    this->currentToken = 0;
    this->currentAddress = 512;
    this->previous = nullptr;
    this->buffer = buffer;
    this->bufferLength = bufferLength;
    this->currentBufferPos = 0;
}

Token *Compiler::advance() {
    if (!isAtEnd())
        currentToken++;
    previous = &tokens->at(currentToken - 1);
    return previous;
}

Token *Compiler::peek() {
    Token *token = &tokens->at(currentToken);
    return token;
}

Token *Compiler::peekNext() { return &tokens->at(currentToken + 1); }

bool Compiler::isAtEnd() { return currentToken >= (int)tokens->size(); }

bool Compiler::check(TokenType type) {
    if (isAtEnd())
        return false;
    return peek()->type == type;
}

bool Compiler::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Compiler::matchBetween(TokenType start, TokenType end) {
    if (isAtEnd())
        return false;
    if (start <= peek()->type && peek()->type <= end) {
        advance();
        return true;
    }
    return false;
}

void Compiler::writeInstruction(uint16_t instruction) {
    if (currentBufferPos + 1 > (bufferLength - 2)) {
        fprintf(stderr, "Assembly file is too large.\n");
        exit(65);
    }
    buffer[currentBufferPos] = (uint8_t)(instruction >> 8);
    currentBufferPos++;
    buffer[currentBufferPos] = (uint8_t)(instruction);
    currentBufferPos++;
}

bool Compiler::consume(TokenType type, const char *message) {
    if (advance()->type != type) {
        error(previous, message);
        return false;
    }
    return true;
}

void Compiler::error(Token *token, const char *message, ...) {
    if (!panicMode) {
        panicMode = true;
        fprintf(stderr, "[line %d] ", token->line);
        va_list args;
        va_start(args, message);
        vfprintf(stderr, message, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
    hadError = true;
}

uint8_t charToHex(char c) {
    if (isDigit(c)) {
        return c - '0';
    } else if (isUpper(c)) {
        return c - 'A' + 10;
    } else if (isLower(c)) {
        return c - 'a' + 10;
    }
    return 199; // something went wrong
}

uint16_t decodeBinaryLiteral(Token *literal) {
    uint16_t num = 0;
    for (int i = 1; i < literal->length; i++) {
        char c = *(literal->start + i);
        num |= (c << (i - 1));
    }
    return num;
}

uint16_t decodeHexLiteral(Token *literal) {
    uint16_t num = 0;
    for (int i = 1; i < literal->length; i++) {
        uint8_t c = charToHex(*(literal->start + i));
        num |= (c << ((literal->length - (i + 1)) * 4));
    }
    return num;
}

uint16_t Compiler::decodeLiteral(Token *literal, uint8_t binaryLength,
                                 uint8_t hexLength, const char *message) {
    uint16_t num = 0;
    if (*literal->start == '%' && (literal->length - 1) == binaryLength) {
        num = decodeBinaryLiteral(literal);
    } else if (*literal->start == '$' && (literal->length - 1) == hexLength) {
        num = decodeHexLiteral(literal);
    } else {
        error(previous, message);
    }
    return num;
}

inline uint16_t extractXReg(Token *token) {
    return (uint16_t)(charToHex(*(token->start + 1)) << 8);
}

inline uint16_t extractYReg(Token *token) {
    return (uint16_t)(charToHex(*(token->start + 1)) << 4);
}
void Compiler::instructionStmt() {
#define CONSUME(tType, message)                                                \
    if (!consume(tType, message)) {                                            \
        break;                                                                 \
    }
#define DOUBLE_REG_INST(inst, opcode)                                          \
    CONSUME(TOKEN_V_REGISTER, "inst expects V register as first argument.");   \
    uint16_t x = extractXReg(previous);                                        \
    CONSUME(TOKEN_COMMA, "Exptected ',' between arguments.");                  \
    CONSUME(TOKEN_V_REGISTER, "inst expects V register as second argument.");  \
    uint16_t y = extractYReg(previous);                                        \
    writeInstruction(opcode + x + y);                                          \
    return;

#define SINGLE_REG_INST(inst, opcode)                                          \
    CONSUME(TOKEN_V_REGISTER, "inst expects V register as argument.");         \
    uint16_t x = extractXReg(previous);                                        \
    writeInstruction(opcode + x);                                              \
    return;

    Token *token = previous;
    switch (token->type) {
        case TOKEN_INST_CLS: {
            if (matchBetween(TOKEN_COMMA, TOKEN_INST_LDV)) {
                error(previous, "Instruction 'CLS' has no arguments.");
            }
            writeInstruction(0x00E0);
            return;
        } break;
        case TOKEN_INST_RET: {
            if (matchBetween(TOKEN_COMMA, TOKEN_INST_LDV)) {
                error(previous, "Instruction 'RET' has no arguments.");
            }
            writeInstruction(0x00EE);
            return;
        } break;
        case TOKEN_INST_JP: {
            uint16_t val = 0;
            if (match(TOKEN_IDENTIFIER)) {
                std::string label(previous->start, previous->length);
                if (labelMap.find(label) != labelMap.end()) {
                    val = labelMap.at(label);
                } else {
                    error(previous, "Label '%.*s' does not exist.",
                          previous->length, previous->start);
                    break;
                }
            } else if (match(TOKEN_LITERAL)) {
                val = decodeLiteral(previous, 12, 3,
                                    "JP instruction expects 12 bit literal.");
            } else {
                error(advance(), "'JP' expects either label, literal or V "
                                 "register as first argument.");
                break;
            }
            writeInstruction(0x1000 + val);
        } break;
        case TOKEN_INST_JPO: {
            uint16_t val = 0;
            if (match(TOKEN_IDENTIFIER)) {
                std::string label(previous->start, previous->length);
                if (labelMap.find(label) != labelMap.end()) {
                    val = labelMap.at(label);
                } else {
                    error(previous, "Label '%.*s' does not exist.",
                          previous->length, previous->start);
                    break;
                }
            } else if (match(TOKEN_LITERAL)) {
                val =
                    decodeLiteral(previous, 12, 3,
                                  "'JPO' instruction expects 12 bit literal.");
            } else {
                error(advance(), "'JPO' expects either label, literal or V "
                                 "register as first argument.");
                break;
            }
            writeInstruction(0xB000 + val);
        } break;
        case TOKEN_INST_CALL: {
            uint16_t val = 0;
            if (match(TOKEN_IDENTIFIER)) {
                std::string label(previous->start, previous->length);
                if (labelMap.find(label) != labelMap.end()) {
                    val = labelMap.at(label);
                } else {
                    error(previous, "Label '%.*s' does not exist.",
                          previous->length, previous->start);
                    break;
                }
            } else if (match(TOKEN_LITERAL)) {
                val = decodeLiteral(previous, 12, 3,
                                    "JMP instruction expects 12 bit address.");
            }
            writeInstruction(0x2000 + val);
            return;
        } break;
        case TOKEN_INST_SE: {
            CONSUME(TOKEN_V_REGISTER,
                    "SE instruction expects V register as first argument.");
            uint16_t x = extractXReg(previous);
            CONSUME(TOKEN_COMMA, "Expected ',' between instruction arguments.");
            if (match(TOKEN_V_REGISTER)) {
                uint16_t y = extractYReg(previous);
                writeInstruction(0x5000 + x + y);
                return;
            } else if (match(TOKEN_LITERAL)) {
                uint16_t val = decodeLiteral(previous, 8, 2, "Expected byte.");
                writeInstruction(0x3000 + x + val);
                return;
            } else if (match(TOKEN_IDENTIFIER)) {
                uint16_t val = 0;
                std::string str(previous->start, previous->length);
                if ((variableMap.find(str) == variableMap.end())) {
                    error(previous, "Variable '%.*s' does not exist.",
                          previous->length, previous->start);
                    break;
                } else if (variableMap.at(str) > 255) {
                    error(previous,
                          "Value %d in variable '%.*s' is too large "
                          "(expected byte).",
                          variableMap.at(str), previous->length,
                          previous->start);
                    break;
                }
                val = variableMap.at(str);
                writeInstruction(0x3000 + x + val);
            } else {
                error(advance(), "Instruction SE expects either V register or "
                                 "literal as second argument.");
                break;
            }
        } break;
        case TOKEN_INST_SNE: {
            CONSUME(TOKEN_V_REGISTER,
                    "'SNE' instruction expects V register as first argument.");
            uint16_t x = extractXReg(previous);
            CONSUME(TOKEN_COMMA, "Expected ',' between instruction arguments.");
            if (match(TOKEN_V_REGISTER)) {
                uint16_t y = extractYReg(previous);
                writeInstruction(0x9000 + x + y);
                return;
            } else if (match(TOKEN_LITERAL)) {
                uint16_t val = decodeLiteral(previous, 8, 2, "Expected byte.");
                writeInstruction(0x4000 + x + val);
                return;
            } else if (match(TOKEN_IDENTIFIER)) {
                uint16_t val = 0;
                std::string str(previous->start, previous->length);
                if ((variableMap.find(str) == variableMap.end())) {
                    error(previous, "Variable '%.*s' does not exist.",
                          previous->length, previous->start);
                    break;
                } else if (variableMap.at(str) > 255) {
                    error(previous,
                          "Value %d in variable '%.*s' is too large "
                          "(expected byte).",
                          variableMap.at(str), previous->length,
                          previous->start);
                    break;
                }
                val = variableMap.at(str);
                writeInstruction(0x4000 + x + val);
            } else {
                error(advance(), "Instruction SNE expects either V register or "
                                 "literal as second argument.");
                break;
            }
        } break;
        case TOKEN_INST_LD: {
            if (match(TOKEN_V_REGISTER)) {
                uint16_t x = extractXReg(previous);
                CONSUME(TOKEN_COMMA, "Expected ',' between arguments.");
                if (match(TOKEN_LITERAL)) {
                    uint16_t byte =
                        decodeLiteral(previous, 8, 2, "Expected byte literal.");
                    writeInstruction(0x6000 + x + byte);
                    return;
                } else if (match(TOKEN_IDENTIFIER)) {
                    std::string str(previous->start, previous->length);
                    if ((variableMap.find(str) == variableMap.end()) ||
                        variableMap.at(str) > 255) {
                        error(previous, "Value in variable is too large for LD "
                                        "(expected byte).");
                        break;
                    }
                    writeInstruction(0x6000 + x + variableMap.at(str));
                    return;
                } else if (match(TOKEN_V_REGISTER)) {
                    uint16_t y = extractYReg(previous);
                    writeInstruction(0x8000 + x + y);
                    return;
                } else {
                    error(advance(), "Expected V register or byte literal.");
                    break;
                }
            } else if (match(TOKEN_I_REGISTER)) {
                CONSUME(TOKEN_COMMA, "Expected ',' between arguments.");
                uint16_t addr = 0;
                if (match(TOKEN_LITERAL)) {
                    addr = decodeLiteral(previous, 12, 3,
                                         "LD expects 12 bit literal.");
                } else if (match(TOKEN_IDENTIFIER)) {
                    std::string str(previous->start, previous->length);
                    if ((variableMap.find(str) == variableMap.end())) {
                        error(previous, "Variable '%.*s' does not exist.",
                              previous->length, previous->start);
                        break;
                    } else if (variableMap.at(str) > 4095) {
                        error(previous,
                              "Value %d in variable '%.*s' is too large "
                              "(expected 12 bit).",
                              variableMap.at(str), previous->length,
                              previous->start);
                        break;
                    }
                    addr = variableMap.at(str);
                }
                writeInstruction(0xA000 + addr);
                return;
            } else {
                error(advance(),
                      "LD expects either V or I register as first argument.");
                break;
            }
        } break;
        case TOKEN_INST_ADD: {
            if (match(TOKEN_V_REGISTER)) {
                uint16_t x = extractXReg(previous);
                CONSUME(TOKEN_COMMA, "Expected ',' between arguments.");
                if (match(TOKEN_V_REGISTER)) {
                    uint16_t y = extractYReg(previous);
                    writeInstruction(0x8004 + x + y);
                    return;
                } else if (match(TOKEN_LITERAL)) {
                    uint16_t byte =
                        decodeLiteral(previous, 8, 2, "Expected byte literal.");
                    writeInstruction(0x7000 + x + byte);
                    return;
                } else if (match(TOKEN_IDENTIFIER)) {
                    std::string str(previous->start, previous->length);
                    if ((variableMap.find(str) == variableMap.end()) ||
                        variableMap.at(str) > 255) {
                        error(previous,
                              "Value in variable is too large for ADD "
                              "(expected byte).");
                        break;
                    }
                    writeInstruction(0x7000 + x + variableMap.at(str));
                    return;
                } else {
                    error(advance(), "Expected V register, byte literal or "
                                     "byte identifier.");
                    break;
                }
            } else if (match(TOKEN_I_REGISTER)) {
                CONSUME(TOKEN_COMMA, "Expected ',' between arguments.");
                CONSUME(TOKEN_V_REGISTER,
                        "ADD expects V register after I register argument.");
                uint16_t x = extractXReg(previous);
                writeInstruction(0xF01E + x);
                return;
            }
        } break;
        case TOKEN_INST_OR: {
            DOUBLE_REG_INST(OR, 0x8001);
        } break;
        case TOKEN_INST_AND: {
            DOUBLE_REG_INST(AND, 0x8002);
        } break;
        case TOKEN_INST_XOR: {
            DOUBLE_REG_INST(XOR, 0x8003);
        } break;
        case TOKEN_INST_SUB: {
            DOUBLE_REG_INST(SUB, 0x8005);
        } break;
        case TOKEN_INST_SHR: {
            SINGLE_REG_INST(SHR, 0x8006);
        } break;
        case TOKEN_INST_SUBN: {
            DOUBLE_REG_INST(SUBN, 0x8007);
        } break;
        case TOKEN_INST_SHL: {
            SINGLE_REG_INST(SHL, 0x800E);
        } break;
        case TOKEN_INST_RND: {
            CONSUME(TOKEN_V_REGISTER,
                    "RND expects V register as first argument.");
            uint16_t x = extractXReg(previous);
            CONSUME(TOKEN_COMMA, "Expected ',' between arguments.");
            uint16_t byte = 0;
            if (match(TOKEN_LITERAL)) {
                byte = decodeLiteral(previous, 8, 2, "RND expects byte.");
            } else if (match(TOKEN_IDENTIFIER)) {
                std::string str(previous->start, previous->length);
                if ((variableMap.find(str) == variableMap.end())) {
                    error(previous, "Variable '%.*s' does not exist.",
                          previous->length, previous->start);
                    break;
                } else if (variableMap.at(str) > 4095) {
                    error(previous,
                          "Value %d in variable '%.*s' is too large "
                          "(expected byte).",
                          variableMap.at(str), previous->length,
                          previous->start);
                    break;
                }
                byte = variableMap.at(str);
            }
            writeInstruction(0xC000 + x + byte);
            return;
        } break;
        case TOKEN_INST_DRW: {
            CONSUME(TOKEN_V_REGISTER,
                    "DRW expects V register as first argument.");
            uint16_t x = extractXReg(previous);
            CONSUME(TOKEN_COMMA, "Expect ',' between arguments.");
            CONSUME(TOKEN_V_REGISTER,
                    "DRW expects V register as second argument.");
            uint16_t y = extractYReg(previous);
            CONSUME(TOKEN_COMMA, "Expect ',' between arguments.");
            uint16_t nibble = 0;
            if (match(TOKEN_LITERAL)) {
                nibble = decodeLiteral(previous, 4, 1,
                                       "'DRW' expected 4 bit literal.");
            } else if (match(TOKEN_IDENTIFIER)) {
                std::string identifier(previous->start, previous->length);
                if (variableMap.find(identifier) == variableMap.end()) {
                    error(previous, "Variable '%.*s' does not exist.",
                          previous->length, previous->start);
                    break;
                } else if (variableMap.at(identifier) > 15) {
                    error(previous,
                          "'DRW' expects nibble, value in variable '%.*s' is "
                          "too large.",
                          previous->length, previous->start);
                    break;
                } else {
                    nibble = variableMap.at(identifier);
                }
            } else {
                error(advance(), "'DRW' expects 4 bit variable or literal as "
                                 "third argument.");
                break;
            }
            writeInstruction(0xD000 + x + y + nibble);
            return;
        } break;
        case TOKEN_INST_SKP: {
            SINGLE_REG_INST(SKP, 0xE09E);
        } break;
        case TOKEN_INST_SKNP: {
            SINGLE_REG_INST(SKNP, 0xE0A1);
        } break;
        case TOKEN_INST_GDT: {
            SINGLE_REG_INST(GDT, 0xF007);
        } break;
        case TOKEN_INST_WKP: {
            SINGLE_REG_INST(WKP, 0xF00A);
        } break;
        case TOKEN_INST_SDT: {
            SINGLE_REG_INST(SDT, 0xF015);
        } break;
        case TOKEN_INST_SST: {
            SINGLE_REG_INST(SST, 0xF018);
        } break;
        case TOKEN_INST_FNT: {
            SINGLE_REG_INST(FNT, 0xF029);
        } break;
        case TOKEN_INST_BCD: {
            SINGLE_REG_INST(BCD, 0xF033);
        } break;
        case TOKEN_INST_STV: {
            SINGLE_REG_INST(STV, 0xF055);
        } break;
        case TOKEN_INST_LDV: {
            SINGLE_REG_INST(LDV, 0xF065);
        } break;
        default: {
            error(previous, "Unexpected token while parsing instruction.");
        } break;
    }

    // this code block SHOULD only be accesible if an error occured
    // in all other cases we should return from the function within the switch
    // statement
    if (panicMode) {
        // hadError = true;
        synchronize();
    }
    return;
#undef CONSUME
#undef DOUBLE_REG_INST
#undef SINGLE_REG_INST
}

void Compiler::assignStmt(Token *identifier) {
    std::string variable(identifier->start, identifier->length);
    if (consume(TOKEN_LITERAL, "Can only assign literals to variable")) {
        uint16_t val = 0;
        if (*previous->start == '%') {
            val = decodeBinaryLiteral(previous);
        } else if (*previous->start == '$') {
            val = decodeHexLiteral(previous);
        }
        variableMap[variable] = val;
    }
}

void Compiler::synchronize() {
    while (!isAtEnd() && !match(TOKEN_NEWLINE)) {
        advance();
    }
}

void Compiler::statement() {
    if (match(TOKEN_IDENTIFIER)) {
        Token *identifier = previous;
        if (match(TOKEN_EQUAL)) {
            assignStmt(identifier);
        } else if (match(TOKEN_COLON)) {
            // labelStmt();
        } else {
            error(peek(), "Expected either '=' or ':' for identifier '%.*s'.",
                  identifier->length, identifier->start);
            synchronize();
        }
    } else if (matchBetween(TOKEN_INST_CLS, TOKEN_INST_LDV)) {
        instructionStmt();
    } else if (match(TOKEN_NEWLINE)) {
        panicMode = false;
    } else {
        error(peek(), "Line has to start with identifier or instruction.");
        synchronize();
    }
}

// loops over the token vector and resolves labels to actual
// addresses in memory
void Compiler::labelPass() {
    while (!isAtEnd()) {
        if (match(TOKEN_IDENTIFIER)) {
            if (check(TOKEN_COLON)) {
                std::string str(previous->start, previous->length);
                labelMap[str] = currentAddress;
                advance();
            }
        } else if (matchBetween(TOKEN_INST_CLS, TOKEN_INST_LDV)) {
            currentAddress += 2;
        }
        synchronize();
    }
    currentToken = 0;
    currentAddress = 512;
    previous = nullptr;
}

int Compiler::compile() {
    labelPass();
    while (!isAtEnd()) {
        statement();
    }
    buffer[currentBufferPos] = EOF;
    return currentBufferPos;
}
