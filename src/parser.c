#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "memory.h"
#include "opcodes.h"
#include "parser.h"

#define LINE_BUFFER_SIZE 128
#define I_TYPE_IMMEDIATE_MIN (-131072)
#define I_TYPE_IMMEDIATE_MAX 131071
#define SHIFT_AMOUNT_MIN 0
#define SHIFT_AMOUNT_MAX 31

// Parse register tokens such as R0, R1, ... R31.
// The parser only translates text into numbers to build the raw instruction.
static bool parse_register(const char *token, uint32_t *value)
{
    if (token == NULL || token[0] != 'R')
    {
        printf("PARSER ERROR: expected register token like R0 but got '%s'\n", token ? token : "(null)");
        return false;
    }

    char *endptr = NULL;
    long reg = strtol(token + 1, &endptr, 10);

    if (endptr == token + 1 || *endptr != '\0' || reg < 0 || reg > 31)
    {
        printf("PARSER ERROR: invalid register '%s', use R0..R31\n", token);
        return false;
    }

    *value = (uint32_t)reg;
    return true;
}

// Parse an immediate value from the instruction text.
// Supports decimal and 0x-prefixed hexadecimal values.
static bool parse_immediate(const char *token, int32_t *value)
{
    if (token == NULL)
    {
        printf("PARSER ERROR: expected immediate token but got null\n");
        return false;
    }

    char *endptr = NULL;
    long parsed_value = strtol(token, &endptr, 0);

    if (endptr == token || *endptr != '\0')
    {
        printf("PARSER ERROR: invalid immediate '%s'\n", token);
        return false;
    }

    *value = (int32_t)parsed_value;
    return true;
}

static bool validate_i_type_immediate(int32_t value, const char *opcode_name)
{
    if (value < I_TYPE_IMMEDIATE_MIN || value > I_TYPE_IMMEDIATE_MAX)
    {
        printf("PARSER ERROR: %s immediate %d is outside 18-bit signed range [%d..%d]\n",
               opcode_name,
               value,
               I_TYPE_IMMEDIATE_MIN,
               I_TYPE_IMMEDIATE_MAX);
        return false;
    }

    return true;
}

static bool validate_shift_amount(int32_t amount, const char *opcode_name)
{
    if (amount < SHIFT_AMOUNT_MIN || amount > SHIFT_AMOUNT_MAX)
    {
        printf("PARSER ERROR: %s shift amount %d is outside allowed range [%d..%d]\n",
               opcode_name,
               amount,
               SHIFT_AMOUNT_MIN,
               SHIFT_AMOUNT_MAX);
        return false;
    }

    return true;
}

static bool ensure_exact_arg_count(const char *opcode_token, char *arg1, char *arg2, char *arg3, char *arg4)
{
    if (arg1 == NULL || arg2 == NULL || arg3 == NULL || arg4 != NULL)
    {
        printf("PARSER ERROR: malformed %s instruction, expected exactly 3 operands\n", opcode_token);
        return false;
    }

    return true;
}

static bool ensure_two_operand_form(const char *opcode_token, char *arg1, char *arg2, char *arg3)
{
    if (arg1 == NULL || arg2 == NULL || arg3 != NULL)
    {
        printf("PARSER ERROR: malformed %s instruction, expected exactly 2 operands\n", opcode_token);
        return false;
    }

    return true;
}

static bool ensure_one_operand_form(const char *opcode_token, char *arg1, char *arg2)
{
    if (arg1 == NULL || arg2 != NULL)
    {
        printf("PARSER ERROR: malformed %s instruction, expected exactly 1 operand\n", opcode_token);
        return false;
    }

    return true;
}

void parse_assembly_file(const char *filepath)
{
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        printf("PARSER ERROR: could not open %s\n", filepath);
        return;
    }

    char line[LINE_BUFFER_SIZE];
    uint32_t instruction_address = 0u;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *cursor = line;

        // Skip whitespace and ignore comments or blank lines.
        while (isspace((unsigned char)*cursor))
        {
            cursor++;
        }

        if (*cursor == '\0' || *cursor == '\n' || *cursor == '#' || *cursor == ';')
        {
            continue;
        }

        char *opcode_token = strtok(cursor, " \t,\n");
        if (opcode_token == NULL)
        {
            continue;
        }

        char *arg1 = strtok(NULL, " \t,\n");
        char *arg2 = strtok(NULL, " \t,\n");
        char *arg3 = strtok(NULL, " \t,\n");
        char *arg4 = strtok(NULL, " \t,\n");

        int opcode = opcode_from_string(opcode_token);
        if (opcode < 0)
        {
            printf("PARSER ERROR: unknown opcode '%s'\n", opcode_token);
            continue;
        }

        // The parser only tokenizes assembly and builds raw machine code.
        // It does not retain decoded fields for later stages.

        instruction_t encoded = 0u;

        switch (opcode)
        {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_AND:
        {
            uint32_t r1 = 0;
            uint32_t r2 = 0;
            uint32_t r3 = 0;
            if (!ensure_exact_arg_count(opcode_token, arg1, arg2, arg3, arg4))
            {
                continue;
            }
            if (!parse_register(arg1, &r1) ||
                !parse_register(arg2, &r2) ||
                !parse_register(arg3, &r3))
            {
                continue;
            }
            encoded = encode_r_type((opcode_t)opcode, r1, r2, r3, 0u);
            break;
        }
        case OPCODE_MOVR:
        {
            int32_t immediate = 0;
            uint32_t r1 = 0;
            uint32_t r2 = 0;
            if (!ensure_exact_arg_count(opcode_token, arg1, arg2, arg3, arg4))
            {
                continue;
            }
            if (!parse_register(arg1, &r1) ||
                !parse_register(arg2, &r2) ||
                !parse_immediate(arg3, &immediate) ||
                !validate_i_type_immediate(immediate, opcode_token))
            {
                continue;
            }
            encoded = encode_i_type((opcode_t)opcode, r1, r2, immediate);
            break;
        }
        case OPCODE_MOVI:
        {
            int32_t immediate = 0;
            uint32_t r1 = 0;
            if (!ensure_two_operand_form(opcode_token, arg1, arg2, arg3))
            {
                continue;
            }
            if (!parse_register(arg1, &r1) ||
                !parse_immediate(arg2, &immediate) ||
                !validate_i_type_immediate(immediate, opcode_token))
            {
                continue;
            }
            encoded = encode_i_type((opcode_t)opcode, r1, 0u, immediate);
            break;
        }
        case OPCODE_ORI:
        {
            int32_t immediate = 0;
            uint32_t r1 = 0;
            uint32_t r2 = 0;
            if (!ensure_exact_arg_count(opcode_token, arg1, arg2, arg3, arg4))
            {
                continue;
            }
            if (!parse_register(arg1, &r1) ||
                !parse_register(arg2, &r2) ||
                !parse_immediate(arg3, &immediate) ||
                !validate_i_type_immediate(immediate, opcode_token))
            {
                continue;
            }
            encoded = encode_i_type((opcode_t)opcode, r1, r2, immediate);
            break;
        }
        case OPCODE_JEQ:
        {
            int32_t offset = 0;
            uint32_t r1 = 0;
            uint32_t r2 = 0;
            if (!ensure_exact_arg_count(opcode_token, arg1, arg2, arg3, arg4))
            {
                continue;
            }
            if (!parse_register(arg1, &r1) ||
                !parse_register(arg2, &r2) ||
                !parse_immediate(arg3, &offset) ||
                !validate_i_type_immediate(offset, opcode_token))
            {
                continue;
            }
            encoded = encode_i_type((opcode_t)opcode, r1, r2, offset);
            break;
        }
        case OPCODE_LSL:
        case OPCODE_LSR:
        {
            int32_t amount = 0;
            uint32_t r1 = 0;
            uint32_t r2 = 0;
            if (!ensure_exact_arg_count(opcode_token, arg1, arg2, arg3, arg4))
            {
                continue;
            }
            if (!parse_register(arg1, &r1) ||
                !parse_register(arg2, &r2) ||
                !parse_immediate(arg3, &amount) ||
                !validate_shift_amount(amount, opcode_token))
            {
                continue;
            }
            encoded = encode_r_type((opcode_t)opcode, r1, r2, 0u, (uint32_t)amount);
            break;
        }
        case OPCODE_JMP:
        {
            int32_t immediate = 0;
            uint32_t address = 0u;
            if (!ensure_one_operand_form(opcode_token, arg1, arg2))
            {
                continue;
            }
            if (!parse_immediate(arg1, &immediate))
            {
                continue;
            }
            if (immediate < 0)
            {
                printf("PARSER ERROR: JMP address %d must be non-negative\n", immediate);
                continue;
            }
            address = (uint32_t)immediate;
            encoded = encode_j_type((opcode_t)opcode, address);
            break;
        }
        case OPCODE_MOVM:
        {
            int32_t immediate = 0;
            uint32_t r1 = 0;
            uint32_t r2 = 0;
            if (!ensure_exact_arg_count(opcode_token, arg1, arg2, arg3, arg4))
            {
                continue;
            }
            if (!parse_register(arg1, &r1) ||
                !parse_register(arg2, &r2) ||
                !parse_immediate(arg3, &immediate) ||
                !validate_i_type_immediate(immediate, opcode_token))
            {
                continue;
            }
            encoded = encode_i_type((opcode_t)opcode, r1, r2, immediate);
            break;
        }
        default:
            printf("PARSER ERROR: unsupported opcode %d\n", opcode);
            continue;
        }

        memory_store_instruction(instruction_address, encoded);
        instruction_address += 1u;

        if (instruction_address >= INSTRUCTION_MEMORY_SIZE)
        {
            printf("PARSER WARNING: reached maximum instruction memory at %u\n", instruction_address);
            break;
        }
    }

    cpu_set_instruction_count((int)instruction_address);
    fclose(file);
}
