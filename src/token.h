#pragma once

#include <vector>

typedef enum {
    TOKEN_NEWLINE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_EQUAL,

    TOKEN_IDENTIFIER, // Variable, Label
    TOKEN_LITERAL,
    TOKEN_V_REGISTER,
    TOKEN_I_REGISTER,

    TOKEN_INST_CLS,
    TOKEN_INST_RET,
    TOKEN_INST_JP,
    TOKEN_INST_JPO,
    TOKEN_INST_CALL,
    TOKEN_INST_SE,
    TOKEN_INST_SNE,
    TOKEN_INST_LD,
    TOKEN_INST_ADD,
    TOKEN_INST_AND,
    TOKEN_INST_OR,
    TOKEN_INST_XOR,
    TOKEN_INST_SUB,
    TOKEN_INST_SHR,
    TOKEN_INST_SUBN,
    TOKEN_INST_SHL,
    TOKEN_INST_RND,
    TOKEN_INST_DRW,
    TOKEN_INST_SKP,
    TOKEN_INST_SKNP,
    TOKEN_INST_GDT,
    TOKEN_INST_WKP,
    TOKEN_INST_SDT,
    TOKEN_INST_SST,
    TOKEN_INST_FNT,
    TOKEN_INST_BCD,
    TOKEN_INST_STV,
    TOKEN_INST_LDV,
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int length;
    int line;
} Token;
