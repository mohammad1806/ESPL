#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>
#include <errno.h>
#include <ctype.h>

#define MAX_FILES 2
#define MAX_NAME_LENGTH 256

int debug_mode = 0;

typedef struct
{
    int fd;
    void *map_start;
    size_t file_size;
    char filename[MAX_NAME_LENGTH];
    Elf32_Ehdr *header;
    Elf32_Shdr *section_headers;
    char *section_str_table;
} ELFFile;

ELFFile elf_files[MAX_FILES] = {0};
int num_files = 0;

void toggle_debug_mode()
{
    debug_mode = !debug_mode;
    printf("Debug mode is now %s\n", debug_mode ? "ON" : "OFF");
}

void print_magic_number(unsigned char *e_ident)
{
    printf("Magic Number: %c%c%c\n", e_ident[1], e_ident[2], e_ident[3]);
}

int validate_elf_file(Elf32_Ehdr *header)
{
    return (header->e_ident[0] == ELFMAG0 &&
            header->e_ident[1] == ELFMAG1 &&
            header->e_ident[2] == ELFMAG2 &&
            header->e_ident[3] == ELFMAG3);
}

void examine_elf_file()
{
    if (num_files >= MAX_FILES)
    {
        printf("Maximum number of files already opened.\n");
        return;
    }

    char filename[MAX_NAME_LENGTH];
    printf("Enter ELF filename: ");
    scanf("%255s", filename);

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("Error opening file");
        return;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    void *map_start = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED)
    {
        perror("Error mapping file");
        close(fd);
        return;
    }

    Elf32_Ehdr *header = (Elf32_Ehdr *)map_start;

    if (!validate_elf_file(header))
    {
        printf("Not a valid ELF file.\n");
        munmap(map_start, file_size);
        close(fd);
        return;
    }

    printf("File: %s\n", filename);
    printf("Magic Number(ASCII): %c%c%c\n", (char)header->e_ident[1], (char)header->e_ident[2], (char)header->e_ident[3]);
    printf("Data Encoding: %s\n",
           header->e_ident[EI_DATA] == ELFDATA2LSB ? "Little Endian" : header->e_ident[EI_DATA] == ELFDATA2MSB ? "Big Endian"
                                                                                                               : "Unknown");
    printf("Entry Point: 0x%x\n", header->e_entry);
    printf("Section Header Table Offset: 0x%x\n", header->e_shoff);
    printf("Number of Section Headers: %d\n", header->e_shnum);
    printf("Size of Section Headers: %d\n", header->e_shentsize);
    printf("Program Header Table Offset: 0x%x\n", header->e_phoff);
    printf("Number of Program Headers: %d\n", header->e_phnum);
    printf("Size of Program Headers: %d\n", header->e_phentsize);

    // Store file information
    ELFFile *curr_file = &elf_files[num_files];
    curr_file->fd = fd;
    curr_file->map_start = map_start;
    curr_file->file_size = file_size;
    curr_file->header = header;
    curr_file->section_headers = (Elf32_Shdr *)((char *)map_start + header->e_shoff);
    curr_file->section_str_table = (char *)map_start +
                                   curr_file->section_headers[header->e_shstrndx].sh_offset;
    strncpy(curr_file->filename, filename, MAX_NAME_LENGTH - 1);

    num_files++;
    printf("\n");

    if (debug_mode)
    {
        for (int i = 0; i < num_files; i++)
        {
            printf("File %d Debug Info:\n", i);
            printf("  Name: %s\n", elf_files[i].filename);
            printf("  File Descriptor: %d\n", elf_files[i].fd);
            printf("  Mapped File Start: %p\n", elf_files[i].map_start);
            printf("  File Size: %zu bytes\n", elf_files[i].file_size);
        }
    }
    printf("\n");
}

void print_section_names()
{
    if (num_files == 0)
    {
        printf("No ELF files opened.\n");
        return;
    }

    for (int f = 0; f < num_files; f++)
    {
        ELFFile *file = &elf_files[f];
        printf("File %s\n", file->filename);

        Elf32_Ehdr *header = file->header;
        Elf32_Shdr *section_headers = file->section_headers;
        char *str_table = file->section_str_table;

        for (int i = 0; i < header->e_shnum; i++)
        {
            printf("[%d] %s 0x%x %d %d %d\n",
                   i,
                   str_table + section_headers[i].sh_name,
                   section_headers[i].sh_addr,
                   section_headers[i].sh_offset,
                   section_headers[i].sh_size,
                   section_headers[i].sh_type);
        }
        printf("\n");
    }
}

void print_symbols()
{
    if (num_files == 0)
    {
        printf("No ELF files opened.\n");
        return;
    }

    for (int f = 0; f < num_files; f++)
    {
        ELFFile *file = &elf_files[f];
        printf("File %s\n", file->filename);

        Elf32_Shdr *symtab_section = NULL;
        Elf32_Shdr *strtab_section = NULL;

        // Find symbol and string tables
        for (int i = 0; i < file->header->e_shnum; i++)
        {
            Elf32_Shdr *section = &file->section_headers[i];
            if (section->sh_type == SHT_SYMTAB)
            {
                // Validate section offsets and sizes
                if (section->sh_offset + section->sh_size > file->file_size ||
                    section->sh_link >= file->header->e_shnum)
                {
                    printf("Invalid symbol table section.\n");
                    break;
                }

                symtab_section = section;
                strtab_section = &file->section_headers[symtab_section->sh_link];
                break;
            }
        }

        if (!symtab_section)
        {
            printf("No symbol table found.\n");
            continue;
        }

        // Additional safety checks
        if (symtab_section->sh_offset + symtab_section->sh_size > file->file_size ||
            strtab_section->sh_offset + strtab_section->sh_size > file->file_size)
        {
            printf("Invalid section offsets.\n");
            continue;
        }

        Elf32_Sym *symbols = (Elf32_Sym *)((char *)file->map_start + symtab_section->sh_offset);
        char *str_table = (char *)file->map_start + strtab_section->sh_offset;
        int num_symbols = symtab_section->sh_size / sizeof(Elf32_Sym);

        if (debug_mode)
        {
            printf("Debug: Symbol Table Size: %d\n", symtab_section->sh_size);
            printf("Debug: Number of Symbols: %d\n", num_symbols);
        }

        for (int i = 0; i < num_symbols; i++)
        {
            // Additional safety check for symbol name offset
            if ((char *)(&symbols[i]) + sizeof(Elf32_Sym) > (char *)file->map_start + file->file_size)
            {
                printf("Symbol table out of bounds.\n");
                break;
            }

            // Validate string table offset
            if (symbols[i].st_name >= strtab_section->sh_size)
            {
                printf("Invalid symbol name offset.\n");
                continue;
            }

            char *sym_name = str_table + symbols[i].st_name;

            // Additional safety check for symbol name
            if (sym_name >= (char *)file->map_start + file->file_size)
            {
                printf("Symbol name out of bounds.\n");
                continue;
            }

            char *section_name = (symbols[i].st_shndx != SHN_UNDEF &&
                                  symbols[i].st_shndx < file->header->e_shnum)
                                     ? file->section_str_table + file->section_headers[symbols[i].st_shndx].sh_name
                                     : "UNDEF";

            printf("[%d] %x %d %s %s\n",
                   i,
                   symbols[i].st_value,
                   symbols[i].st_shndx,
                   section_name,
                   sym_name);
        }
        printf("\n");
    }
    
}

int is_empty_or_spaces(const char *str) {
    if (str == NULL) return 1; // Consider NULL as "empty"

    while (*str) {
        if (!isspace((unsigned char)*str)) {
            return 0; // Found a non-space character
        }
        str++;
    }

    return 1; // All characters are spaces (or the string is empty)
}

void check_files_for_merge()
{
    if (num_files != 2)
    {
        printf("Exactly 2 ELF files must be opened.\n");
        return;
    }

    ELFFile *file1 = &elf_files[0];
    ELFFile *file2 = &elf_files[1];

    // Find symbol tables
    Elf32_Shdr *symtab1 = NULL, *symtab2 = NULL;
    Elf32_Shdr *strtab1 = NULL, *strtab2 = NULL;

    for (int i = 0; i < file1->header->e_shnum; i++)
    {
        if (file1->section_headers[i].sh_type == SHT_SYMTAB)
        {
            symtab1 = &file1->section_headers[i];
            strtab1 = &file1->section_headers[symtab1->sh_link];
            break;
        }
    }

    for (int i = 0; i < file2->header->e_shnum; i++)
    {
        if (file2->section_headers[i].sh_type == SHT_SYMTAB)
        {
            symtab2 = &file2->section_headers[i];
            strtab2 = &file2->section_headers[symtab2->sh_link];
            break;
        }
    }

    if (!symtab1 || !symtab2)
    {
        printf("Feature not supported: Symbol table not found.\n");
        return;
    }

    Elf32_Sym *symbols1 = (Elf32_Sym *)((char *)file1->map_start + symtab1->sh_offset);
    Elf32_Sym *symbols2 = (Elf32_Sym *)((char *)file2->map_start + symtab2->sh_offset);
    char *str_table1 = (char *)file1->map_start + strtab1->sh_offset;
    char *str_table2 = (char *)file2->map_start + strtab2->sh_offset;

    int num_symbols1 = symtab1->sh_size / sizeof(Elf32_Sym);
    int num_symbols2 = symtab2->sh_size / sizeof(Elf32_Sym);

    // Check symbols in first file
    for (int i = 1; i < num_symbols1; i++)
    {
        char *sym_name = str_table1 + symbols1[i].st_name;
        int found_in_second = 0;

        // Check if undefined symbol
        if (symbols1[i].st_shndx == SHN_UNDEF)
        {
            // Search in second file
            // int found_defined = 0;
            for (int j = 1; j < num_symbols2; j++)
            {
                if (strcmp(sym_name, str_table2 + symbols2[j].st_name) == 0)
                {
                    if (symbols2[j].st_shndx != SHN_UNDEF)
                    {
                        // found_defined = 1;
                        found_in_second = 1;
                        break;
                    }
                }
            }

            if (!found_in_second)
            {
                printf("Symbol %s undefined\n", sym_name);
            }
        }
        // Check for multiply defined symbols
        else
        {
            for (int j = 1; j < num_symbols2; j++)
            {
                if (strcmp(sym_name, str_table2 + symbols2[j].st_name) == 0 &&
                    symbols2[j].st_shndx != SHN_UNDEF && !is_empty_or_spaces(sym_name))
                {
                    printf("Symbol %s multiply defined\n", sym_name);
                    break;
                }
            }
        }
    }
    printf("\n");
}

void merge_elf_files()
{
    if (num_files != 2)
    {
        printf("Exactly 2 ELF files must be opened.\n");
        return;
    }

    ELFFile *file1 = &elf_files[0];
    ELFFile *file2 = &elf_files[1];

    // Open output file
    int out_fd = open("out.ro", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (out_fd == -1)
    {
        perror("Error creating output file");
        return;
    }

    // Copy ELF header from first file
    Elf32_Ehdr merged_header = *file1->header;

    // Create a copy of section headers from first file
    Elf32_Shdr *merged_section_headers = malloc(file1->header->e_shnum * sizeof(Elf32_Shdr));
    memcpy(merged_section_headers, file1->section_headers,
           file1->header->e_shnum * sizeof(Elf32_Shdr));

    // Track current file offset for writing sections
    off_t current_offset = sizeof(Elf32_Ehdr);

    // Write initial ELF header (will be updated later)
    write(out_fd, &merged_header, sizeof(Elf32_Ehdr));

    // Process and merge sections
    for (int i = 0; i < file1->header->e_shnum; i++)
    {
        Elf32_Shdr *section = &merged_section_headers[i];
        char *section_name = file1->section_str_table + section->sh_name;

        // Mergeable sections: .text, .data, .rodata
        if (strcmp(section_name, ".text") == 0 ||
            strcmp(section_name, ".data") == 0 ||
            strcmp(section_name, ".rodata") == 0)
        {

            // Find corresponding section in second file
            Elf32_Shdr *section2 = NULL;
            for (int j = 0; j < file2->header->e_shnum; j++)
            {
                char *section2_name = file2->section_str_table + file2->section_headers[j].sh_name;
                if (strcmp(section_name, section2_name) == 0)
                {
                    section2 = &file2->section_headers[j];
                    break;
                }
            }

            if (!section2)
            {
                printf("Corresponding section not found in second file.\n");
                continue;
            }

            // Update section offset and size in merged section headers
            section->sh_offset = current_offset;
            section->sh_size += section2->sh_size;

            // Write section from first file
            write(out_fd,
                  (char *)file1->map_start + file1->section_headers[i].sh_offset,
                  file1->section_headers[i].sh_size);

            // Write section from second file
            write(out_fd,
                  (char *)file2->map_start + section2->sh_offset,
                  section2->sh_size);

            // Update current offset
            current_offset += file1->section_headers[i].sh_size + section2->sh_size;
        }
        // Non-mergeable sections: copy as-is from first file
        else
        {
            section->sh_offset = current_offset;

            write(out_fd,
                  (char *)file1->map_start + file1->section_headers[i].sh_offset,
                  file1->section_headers[i].sh_size);

            current_offset += file1->section_headers[i].sh_size;
        }
    }

    // Write section header table
    merged_header.e_shoff = current_offset;
    write(out_fd, merged_section_headers,
          file1->header->e_shnum * sizeof(Elf32_Shdr));

    // Update and rewrite ELF header with correct section header offset
    lseek(out_fd, 0, SEEK_SET);
    write(out_fd, &merged_header, sizeof(Elf32_Ehdr));

    // Cleanup
    free(merged_section_headers);
    close(out_fd);

    printf("Merged ELF file created: out.ro\n");
}

typedef struct
{
    char *name;
    void (*function)();
} Menu;

Menu menu_options[] = {
    {"Toggle Debug Mode", toggle_debug_mode},
    {"Examine ELF File", examine_elf_file},
    {"Print Section Names", print_section_names},
    {"Print Symbols", print_symbols},
    {"Check Files for Merge", check_files_for_merge},
    {"Merge ELF Files", merge_elf_files},
    {"Quit", NULL}};

void cleanup_files()
{
    for (int i = 0; i < num_files; i++)
    {
        if (elf_files[i].map_start)
        {
            munmap(elf_files[i].map_start, elf_files[i].file_size);
        }
        if (elf_files[i].fd != -1)
        {
            close(elf_files[i].fd);
        }
    }
}

int main()
{
    int choice;
    while (1)
    {
        printf("Choose action:\n");
        for (int i = 0; i < sizeof(menu_options) / sizeof(Menu); i++)
        {
            if (menu_options[i].name != NULL)
                printf("%d-%s\n", i, menu_options[i].name);
        }

        scanf("%d", &choice);
        getchar();

        if (choice < 0 || choice >= 7)
        {
            printf("Invalid choice\n");
            continue;
        }

        if (menu_options[choice].function == NULL)
        {
            cleanup_files();
            break;
        }

        menu_options[choice].function();
    }

    return 0;
}