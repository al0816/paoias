#include "qwertyuiop.h"

static uint32_t registers[0x20];
static int flags[0x10];
static uint32_t commands[0x400];
static uint32_t data[0x400];

static void init_array() {
    uint32_t *base = data;
    *(base++) = 0xf;
    *(base++) = 0x1742;
    *(base++) = 0x5623;
    *(base++) = 0x1337;
    *(base++) = 0x1488;
    *(base++) = 0x936;
    *(base++) = 0xaaaa;
    *(base++) = 0x5159;
    *(base++) = 0x863;
    *(base++) = 0x1608;
    *(base++) = 0x7332;
    *(base++) = 0x2020;
    *(base++) = 0x7721;
    *(base++) = 0xded0;
    *(base++) = 0xaaaa;
    *(base++) = 0xaaa;
}

static int verbosity = 0;

static inline uint32_t cmd_code(uint32_t cmd) {
    return cmd >> 16;
}

static inline uint32_t op(uint32_t cmd) {
    return cmd & 0xffff;
}

static inline uint32_t *addr(uint16_t offset) {
    return (uint32_t *)((char*)&data + offset);
}

static inline uint32_t *cmd_addr(uint16_t offset) {
    return (uint32_t *)((char*)&commands + offset);
}

static inline void compare(uint32_t op1, uint32_t op2) {
    uint32_t res = op1 - op2;
    flags[zf] = !res;
    flags[sf] = res >> 31;
}

static inline void test(uint32_t op1, uint32_t op2) {
    compare(op1 & op2, 0);
}

static inline void execute_command() {
    const uint32_t command = *cmd_addr(registers[eip]);
    switch (cmd_code(command)) {
        case mov_mem:
            registers[eax] = *addr(op(command));
            break;
        case mov_to:
            registers[op(command)] = registers[eax];
            break;
        case mov_l:
            registers[eax] = op(command);
            break;
        case mov_reg:
            registers[eax] = registers[op(command)];
            break;
        case mov_t:
            registers[eax] = *addr(registers[op(command)]);
            break;
        case test_r:
            test(registers[eax], registers[op(command)]);
            break;
        case jz_l:
            if (flags[zf])
                 registers[eip] += op(command);
            break;
        case add_to:
            registers[op(command)] += registers[eax];
            break;
        case sub_to:
            registers[op(command)] -= registers[eax];
            break;
        case halt:
            print_state();
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "ERROR: invalid command %x\n", command);
            exit(EXIT_FAILURE);
    }

    registers[eip] += sizeof(uint32_t);
}

void exec_loop() {
    while (1) {
        if (verbosity) print_state();
        execute_command();
        if (verbosity) getchar();
    }
}

void print_state() {
    printf("EAX:\t%#010x", registers[eax]);
    printf("\tEBX:\t%#010x", registers[ebx]);
    printf("\tECX:\t%#010x", registers[ecx]);
    printf("\tZF:\t%x\n", flags[zf]);
    printf("EDX:\t%#010x", registers[edx]);
    printf("\tEIP:\t%#010x", registers[eip]);
    printf("\tCMD:\t%#010x", *cmd_addr(registers[eip]));
    printf("\tSF:\t%x\n", flags[sf]);
}

static inline uint16_t hex_to_uint16(const char * const hex) {
    return strtoul(hex, NULL, 16);
}

static inline uint16_t str_to_reg(const char * const str) {
    if (!strcmp(str, "eax")) return eax;
    if (!strcmp(str, "ebx")) return ebx;
    if (!strcmp(str, "ecx")) return ecx;
    if (!strcmp(str, "edx")) return edx;
    if (!strcmp(str, "eip")) return eip;
    return 0;
}

static inline uint32_t cmd_r(int code, char *op) {
    uint32_t command = 0;
    command |= code << 16;
    command |= str_to_reg(op);
    return command;
}

static inline uint32_t cmd_l(int code, char *op) {
    uint32_t command = 0;
    command |= code << 16;
    command |= hex_to_uint16(op);
    return command;
}

static inline void write_exec_file() {
    FILE *executable = fopen("executable", "wb");
    if (!executable) {
        fprintf(stderr, "ERROR: Count not open file");
        exit(EXIT_FAILURE);
    }
    fwrite(commands, 0xfff, 1, executable);
    fclose(executable);
}

static inline void read_exec_file(const char * const filename) {
    FILE *executable = fopen(filename, "rb");
    if (!executable) {
        fprintf(stderr, "ERROR: Could not opne file %s", filename);
        exit(EXIT_FAILURE);
    }
    fread(commands, 0xfff, 1, executable);
    fclose(executable);
}

void parse(const char * const filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: Could not opne file %s", filename);
        exit(EXIT_FAILURE);
    }

    char *line = malloc(1);
    size_t len = 0;
    ssize_t read;

    char cmd[16], op[16];

    uint32_t *command_pointer = commands;

    while ((read = getline(&line, &len, fp)) != -1) {
        if (read < 5 || line[0] == '/')
            continue;
        sscanf(line, "%s %s", cmd, op);

        uint32_t command = 0;
        
        if (!strcmp("MOV_MEM", cmd)) {
            command = cmd_l(mov_mem, op);
        } else if (!strcmp("MOV_TO", cmd)) {
            command = cmd_r(mov_to, op);
        } else if (!strcmp("MOV_L", cmd)) {
            command = cmd_l(mov_l, op);
        } else if (!strcmp("MOV_REG", cmd)) {
            command = cmd_r(mov_reg, op);
        } else if (!strcmp("MOV_T", cmd)) {
            command = cmd_r(mov_t, op);
        } else if (!strcmp("ADD_TO", cmd)) {
            command = cmd_r(add_to, op);
        } else if (!strcmp("SUB_TO", cmd)) {
            command = cmd_r(sub_to, op);
        } else if (!strcmp("TEST_R", cmd)) {
            command = cmd_r(test_r, op);
        } else if (!strcmp("JZ_R", cmd)) {
            command = cmd_l(jz_l, op);
        } else if (!strcmp("HALT", cmd)) {
            command |= halt << 16;
        } else {
            fprintf(stderr, "ERROR: Not implemented\n");
        }

        if (verbosity > 1)
            printf("Command: %s, OP1: %s; %x\n", cmd, op, command);

        *(command_pointer++) = command;
    }

    fclose(fp);
    if (line)
        free(line);
}

static inline void help(const char * const name) {
    printf("usage: %s [OPTIONS]\n", name);
    puts("  -c, --compile\tcompile asm\n" \
         "  -i, --interpret\trun asm\n" \
         "  -r, --run\trun executable\n" \
         "  -h, --help\tprint help");
}

int main(int argc, char **argv) {
    int opt, long_index;
    init_array();

    if (argc == 1) {
        help(argv[0]);
        exit(EXIT_SUCCESS);
    }

    while (optind < argc) {
        if ((opt = getopt_long(argc, argv, opts, long_options, &long_index)) == -1) {
            ++optind;
            continue;
        }
        switch (opt) {
            case 'v':
                ++verbosity;
                break;
            case 'c':
                parse(optarg);
                write_exec_file();
                break;
            case 'i':
                parse(optarg);
                exec_loop();
                break;
            case 'r':
                read_exec_file(optarg);
                exec_loop();
                break;
            case 'h':
                help(argv[0]);
                break;
        }
    }

    return EXIT_SUCCESS;
}
