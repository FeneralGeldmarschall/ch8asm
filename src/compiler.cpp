#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include "compiler.h"
#include "token.h"

Compiler::Compiler(std::vector<Token> *tokens, const char *outfile,
                   char *buffer, int bufferLength) {
    this->tokens = tokens;
    this->currentToken = 0;
    this->currentAddress = 512;
    this->previous = nullptr;
    this->outfile = fopen(outfile, "wb");
    if (this->outfile == NULL) {
        fprintf(stderr, "Could not open file %s", outfile);
        exit(74);
    }
    this->buffer = buffer;
    this->bufferLength = bufferLength;
    this->currentBufferPos = 0;
}

Token *Compiler::advance() {
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

bool Compiler::match(TokenType type) {
    if (isAtEnd())
        return false;
    if (peek()->type == type)
        return true;
    return false;
}

bool Compiler::matchBetween(TokenType start, TokenType end) {
    if (isAtEnd())
        return false;
    if (start <= peek()->type && peek()->type <= end)
        return true;
    return false;
}

void Compiler::writeInstruction(uint16_t instruction) {
    // int ret = fwrite(&instruction, sizeof(uint16_t), 1, outfile);
    // if (ret < 1) {
    //     fprintf(stderr, "Could not write instructions to file.");
    //     exit(74);
    // }
    if (currentBufferPos + 1 > (bufferLength - 2)) {
        fprintf(stderr, "Assembly file is too large.\n");
        exit(65);
    }
    buffer[currentBufferPos] = (char)(instruction >> 8);
    currentBufferPos++;
    buffer[currentBufferPos] = (char)(instruction & 0xFF);
    currentBufferPos++;
}

bool Compiler::consume(TokenType type, const char *message) {
    if (advance()->type != type) {
        errorAtPrevious(message);
        return false;
    }
    return true;
}

void Compiler::errorAtPrevious(const char *message) {
    error(previous, message);
}

void Compiler::error(Token *token, const char *message) {
    if (!panicMode) {
        panicMode = true;
        fprintf(stderr, "[line %d] %s\n", token->line, message);
    }
    hadError = true;
}

// static TokenType checkKeyword(int start, int length, const char *rest,
//                               TokenType type) {
//     if (scanner.current - scanner.start == start + length &&
//         memcmp(scanner.start + start, rest, length) == 0) {
//         return type;
//     }

//     return TOKEN_IDENTIFIER;
// }

// InstructionType checkInstruction(Token *token) {
//     char c = *token->start;
//     switch (c) {
//         case 'A': {
//         } break;
//         case 'C': {
//         } break;
//         case 'D': {
//         } break;
//         case 'J': {
//         } break;
//         case 'L': {} break;
//         case 'O': {} break;
//         case 'R': {} break;
//         case ''
//     }
// }

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
        char c = *(literal->start + i);
        num |= (c << ((i - 1) * 4));
    }
    return num;
}

bool Compiler::decodeLiteral(Token *literal, uint8_t binaryLength,
                             uint8_t hexLength, const char *message) {
    uint16_t num = 0;
    if (*literal->start == '%' && (literal->length - 1) == binaryLength) {
        num = decodeBinaryLiteral(literal);
    } else if (*literal->start == '$' && (literal->length - 1) == hexLength) {
        num = decodeHexLiteral(literal);
    } else {
        errorAtPrevious(message);
    }
    return num;
}

inline uint16_t extractXReg(Token *token) {
    return (uint16_t)((*token->start + 1) << 8);
}

inline uint16_t extractYReg(Token *token) {
    return (uint16_t)((*token->start + 1) << 4);
}
void Compiler::instructionStmt() {
#define EXTRACT_X_REG(token) (uint16_t)(*(token->start + 1) << 8)
#define EXTRACT_Y_REG(token) (uint16_t)(*(token->start + 1) << 4)
#define CONSUME(tType, message)                                                \
    if (peek()->type != tType) {                                               \
        panicMode = true;                                                      \
        fprintf(stderr, "%s", message);                                        \
        advance();                                                             \
        goto error;                                                            \
    } else {                                                                   \
        advance();                                                             \
    }
#define DOUBLE_REG_INST(inst, opcode)                                          \
    CONSUME(TOKEN_V_REGISTER, "inst expects V register as first argument.");   \
    uint16_t x = extractXReg(previous);                                        \
    CONSUME(TOKEN_COMMA, "Exptected ',' between arguments.");                  \
    CONSUME(TOKEN_V_REGISTER, "inst expects V register as second argument.");  \
    uint16_t y = extractYReg(previous);                                        \
    writeInstruction(opcode + x + y);

#define SINGLE_REG_INST(inst, opcode)                                          \
    CONSUME(TOKEN_V_REGISTER, "inst expects V register as argument.");         \
    uint16_t x = extractXReg(previous);                                        \
    writeInstruction(opcode + x);

#define CHECK_ERROR                                                            \
    if (!status)                                                               \
        return;
    // if status wasnt set to false we return from the function and continue
    // to the error handling code

    Token *token = advance();
    // bool status = true;
    switch (token->type) {
        case TOKEN_INST_CLS: {
            consume(TOKEN_NEWLINE, "Instruction 'CLSS' has no arguments.");
            writeInstruction(0x00E0);
        } break;
        case TOKEN_INST_RET: {
            CONSUME(TOKEN_NEWLINE, "Instruction 'RET' has no arguments.");
            writeInstruction(0x00EE);
        } break;
        case TOKEN_INST_JP: {
            Token *next = advance();
            uint16_t val = 0;
            if (match(TOKEN_IDENTIFIER)) {
                std::string label(next->start, next->length);
                if (labelMap.find(label) != labelMap.end()) {
                    labelMap.at(label);
                    writeInstruction(0x1000 + val);
                } else {
                    error(next, "Label does not exist.");
                }

            } else if (match(TOKEN_LITERAL)) {
                val = decodeLiteral(next, 12, 3,
                                    "JMP instruction expects 12 bit address.");
                writeInstruction(0x1000 + val);
            }
        } break;
        case TOKEN_INST_CALL: {
            Token *next = advance();
            uint16_t val = 0;
            if (match(TOKEN_IDENTIFIER)) {
                std::string label(next->start, next->length);
                if (labelMap.find(label) != labelMap.end()) {
                    labelMap.at(label);
                    writeInstruction(0x1000 + val);
                } else {
                    error(next, "Label does not exist.");
                }
            } else if (next->type == TOKEN_LITERAL) {
                val = decodeLiteral(next, 12, 3,
                                    "JMP instruction expects 12 bit address.");
                writeInstruction(0x1000 + val);
            }
        } break;
        case TOKEN_INST_SE: {
            // consume(TOKEN_V_REGISTER,
            // "SE instruction expects V register as first argument.");
            CONSUME(TOKEN_V_REGISTER,
                    "SE instruction expects V register as first argument.");
            Token *vx = previous;
            uint16_t x = EXTRACT_X_REG(vx);
            // consume(TOKEN_COMMA, "Expected ',' between instruction
            // arguments.");
            CONSUME(TOKEN_COMMA, "Expected ',' between instruction arguments.");
            if (match(TOKEN_V_REGISTER)) {
                Token *vy = advance();
                uint16_t y = extractXReg(vy);
                writeInstruction(0x5000 + x + y);
            } else if (match(TOKEN_LITERAL)) {
                uint16_t val = decodeLiteral(advance(), 8, 2, "Expected byte.");
                writeInstruction(0x3000 + x + val);
            } else {
                error(peek(), "Instruction SE expects either V register or "
                              "literal as second argument.");
                advance();
                goto error;
            }
        } break;
        case TOKEN_INST_SNE: {
            CONSUME(TOKEN_V_REGISTER,
                    "SNE instruction expects V register as first argument.");
            CONSUME(TOKEN_COMMA, "Expect ',' between arguments.");
            uint16_t x = EXTRACT_X_REG(previous);
            CONSUME(TOKEN_LITERAL,
                    "SNE instruction expects byte as second argument.");
            uint16_t byte =
                decodeLiteral(previous, 8, 2, "SNE epects byte literal.");
            writeInstruction(0x4000 + x + byte);
        } break;
        case TOKEN_INST_LD: {
            if (match(TOKEN_V_REGISTER)) {
                uint16_t x = extractXReg(advance());
                CONSUME(TOKEN_COMMA, "Expected ',' between arguments.");
                if (match(TOKEN_V_REGISTER)) {
                    uint16_t y = extractYReg(advance());
                    writeInstruction(0x6000 + x + y);
                } else if (match(TOKEN_LITERAL)) {
                    uint16_t byte = decodeLiteral(advance(), 8, 2,
                                                  "Expected byte literal.");
                    writeInstruction(0x8000 + x + byte);
                } else {
                    error(advance(), "Expected V register or byte literal.");
                    goto error;
                }
            } else if (match(TOKEN_I_REGISTER)) {
                CONSUME(TOKEN_LITERAL,
                        "LD expects 12 bit literal after I register.");
                uint16_t addr = decodeLiteral(previous, 12, 3,
                                              "LD expects 12 bit literal.");
                writeInstruction(0xA000 + addr);
            } else {
                error(peek(),
                      "LD expects either V or I register as first argument.");
                goto error;
            }
        } break;
        case TOKEN_INST_ADD: {
            if (match(TOKEN_V_REGISTER)) {
                uint16_t x = extractXReg(advance());
                CONSUME(TOKEN_COMMA, "Expected ',' between arguments.");
                if (match(TOKEN_V_REGISTER)) {
                    uint16_t y = extractYReg(advance());
                    writeInstruction(0x8004 + x + y);
                } else if (match(TOKEN_LITERAL)) {
                    uint16_t byte = decodeLiteral(advance(), 8, 2,
                                                  "Expected byte literal.");
                    writeInstruction(0x7000 + x + byte);
                } else {
                    error(advance(), "Expected V register or byte literal.");
                    goto error;
                }
            } else if (match(TOKEN_I_REGISTER)) {
                CONSUME(TOKEN_COMMA, "Expected ',' between arguments.");
                CONSUME(TOKEN_V_REGISTER,
                        "ADD expects V register after I register argument.");
                uint16_t x = extractXReg(previous);
                writeInstruction(0xF01E + x);
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
            SINGLE_REG_INST(SHL, 0x8008);
        } break;
        case TOKEN_INST_RND: {
            CONSUME(TOKEN_V_REGISTER,
                    "RND expects V register as first argument.");
            uint16_t x = extractXReg(previous);
            CONSUME(TOKEN_COMMA, "Expected ',' between arguments.");
            CONSUME(TOKEN_LITERAL, "RND expects byte as second argument.");
            uint16_t byte = decodeLiteral(previous, 8, 2, "RND expects byte.");
            writeInstruction(0xC000 + x + byte);
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
            CONSUME(TOKEN_LITERAL,
                    "DRW expects nibble literal as third argument.");
            uint16_t nibble =
                decodeLiteral(previous, 4, 1, "Expected 4 bit literal.");
            writeInstruction(0xD000 + x + y + nibble);
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
            errorAtPrevious("Unexpected token while parsing instruction.");
            goto error;
        } break;
    }

    return;

error:
    hadError = true;
    synchronize();
    return;
#undef EXTRACT_X_REG
#undef EXTRACT_Y_REG
#undef CONSUME
#undef DOUBLE_REG_INST
}

void Compiler::assignStmt() {
    std::string variable(previous->start, previous->length);
    advance();
    if (consume(TOKEN_LITERAL, "Can only assign literals to variable.")) {
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
        advance();
        if (match(TOKEN_EQUAL)) {
            assignStmt();
        } else if (match(TOKEN_COLON)) {
            // labelStmt();
            advance();
        } else {
            // error
            error(peek(), "Expected either '=' or ':' for identifier token.");
            synchronize();
        }
    } else if (matchBetween(TOKEN_INST_CLS, TOKEN_INST_LDV)) {
        instructionStmt();
    } else if (match(TOKEN_NEWLINE)) {
        panicMode = false;
        advance();
    } else {
        // error: expected label or assignment
        error(peek(), "Line has to start with identifier or instruction.");
        synchronize();
    }
}

// loops over the token vector and resolves labels to actual
// addresses in memory
void Compiler::labelPass() {
    while (!isAtEnd()) {
        if (match(TOKEN_IDENTIFIER)) {
            advance();
            if (match(TOKEN_COLON)) {
                std::string str;
                for (int i = 0; i < previous->length; i++) {
                    str.assign(previous->start + i);
                }
                labelMap[str] = currentAddress;
            }
            while (!match(TOKEN_NEWLINE))
                advance();
        } else if (matchBetween(TOKEN_INST_CLS, TOKEN_INST_LDV)) {
            currentAddress += 2;
            while (!match(TOKEN_NEWLINE))
                advance();
        }
        advance();
    }
    currentToken = 0;
    currentAddress = 512;
    previous = nullptr;
}

void Compiler::compile() {
    labelPass();
    while (!isAtEnd()) {
        statement();
    }
    buffer[currentBufferPos] = '\0';
}
