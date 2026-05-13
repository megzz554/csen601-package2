#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "memory.h"
#include "opcodes.h"
#include "parser.h"

#define LINE_BUFFER_SIZE 128

// Parse register tokens such as R0, R1, ... R31.
// The parser only translates text into numbers to build the raw instruction.
static uint32_t parse_register(const char *token)
{
    if (token == NULL || token[0] != 'R')
    {
        printf("PARSER ERROR: expected register token like R0 but got '%s'\n", token ? token : "(null)");
        return 0u;
    }

    char *endptr = NULL;
    long reg = strtol(token + 1, &endptr, 10);

    if (endptr == token + 1 || reg < 0 || reg > 31)
    {
        printf("PARSER ERROR: invalid register '%s', use R0..R31\n", token);
        return 0u;
    }

    return (uint32_t)reg;
}

// Parse an immediate value from the instruction text.
// Supports decimal and 0x-prefixed hexadecimal values.
static int32_t parse_immediate(const char *token)
{
    if (token == NULL)
    {
        printf("PARSER ERROR: expected immediate token but got null\n");
        return 0;
    }

    char *endptr = NULL;
    long value = strtol(token, &endptr, 0);

    if (endptr == token)
    {
        printf("PARSER ERROR: invalid immediate '%s'\n", token);
        return 0;
    }

    return (int32_t)value;
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
            uint32_t rd = parse_register(arg1);
            uint32_t rs = parse_register(arg2);
            uint32_t rt = parse_register(arg3);
            encoded = encode_r_type((opcode_t)opcode, rs, rt, rd);
            break;
        }
        case OPCODE_MOVR:
        {
            if (arg3 != NULL && arg3[0] == 'R')
            {
                uint32_t rd = parse_register(arg1);
                uint32_t rs = parse_register(arg2);
                uint32_t rt = parse_register(arg3);
                encoded = encode_r_type((opcode_t)opcode, rs, rt, rd);
            }
            else
            {
                uint32_t r1 = parse_register(arg1);
                uint32_t r2 = parse_register(arg2);
                int32_t immediate = parse_immediate(arg3);
                encoded = encode_i_type((opcode_t)opcode, r2, r1, immediate);
            }
            break;
        }
        case OPCODE_MOVI:
        {
            uint32_t rt = parse_register(arg1);
            int32_t immediate = parse_immediate(arg2);
            encoded = encode_i_type((opcode_t)opcode, 0u, rt, immediate);
            break;
        }
        case OPCODE_XORI:
        {
            uint32_t rt = parse_register(arg1);
            uint32_t rs = parse_register(arg2);
            int32_t immediate = parse_immediate(arg3);
            encoded = encode_i_type((opcode_t)opcode, rs, rt, immediate);
            break;
        }
        case OPCODE_JEQ:
        {
            uint32_t rs = parse_register(arg1);
            uint32_t rt = parse_register(arg2);
            int32_t offset = parse_immediate(arg3);
            encoded = encode_i_type((opcode_t)opcode, rs, rt, offset);
            break;
        }
        case OPCODE_LSL:
        case OPCODE_LSR:
        {
            uint32_t rt = parse_register(arg1);
            uint32_t rs = parse_register(arg2);
            int32_t amount = parse_immediate(arg3);
            encoded = encode_i_type((opcode_t)opcode, rs, rt, amount);
            break;
        }
        case OPCODE_JMP:
        {
            uint32_t address = (uint32_t)parse_immediate(arg1);
            encoded = encode_j_type((opcode_t)opcode, address);
            break;
        }
        case OPCODE_MOVM:
        {
            uint32_t r1 = parse_register(arg1);
            uint32_t r2 = arg3 == NULL ? 0u : parse_register(arg2);
            int32_t immediate = parse_immediate(arg3 == NULL ? arg2 : arg3);
            encoded = encode_i_type((opcode_t)opcode, r2, r1, immediate);
            break;
        }
        default:
            printf("PARSER ERROR: unsupported opcode %d\n", opcode);
            continue;
        }

        printf("PARSE: '%s' -> 0x%08X\n", line, encoded);
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
