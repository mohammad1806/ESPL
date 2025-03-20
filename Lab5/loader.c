#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <elf.h>
#include <string.h>


void load_phdr(Elf32_Phdr *phdr, int fd) ;
extern int startup(int argc, char **argv, void (*start)());

// Function to get the type name in uppercase
const char *get_phdr_type_name(int type) {
    switch (type) {
        case 0: return "NULL";         // Unused segment.
        case 1: return "LOAD";         // Loadable segment.
        case 2: return "DYNAMIC";      // Dynamic linking information.
        case 3: return "INTERP";       // Interpreter pathname.
        case 4: return "NOTE";         // Auxiliary information.
        case 5: return "SHLIB";        // Reserved.
        case 6: return "PHDR";         // The program header table itself.
        case 7: return "TLS";          // The thread-local storage template.
        default: return "UNKNOWN";     // Unrecognized type.
    }
}


// Function to get protection flags as a string
const char *get_prot_flags(Elf32_Word flags) {
    static char prot[4] = "---";  // Default to no permissions
    prot[0] = (flags & PF_R) ? 'R' : '-';
    prot[1] = (flags & PF_W) ? 'W' : '-';
    prot[2] = (flags & PF_X) ? 'X' : '-';
    prot[3] = '\0';
    return prot;
}

// Function to calculate mmap protection flags
int calculate_mmap_prot(Elf32_Word flags) {
    int prot = 0;
    if (flags & PF_R) prot |= PROT_READ;
    if (flags & PF_W) prot |= PROT_WRITE;
    if (flags & PF_X) prot |= PROT_EXEC;
    return prot;
}

// Iterator function
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg) {
    // Ensure the file starts with a valid ELF header
    Elf32_Ehdr *elf_header = (Elf32_Ehdr *)map_start;
    if (elf_header->e_ident[EI_MAG0] != ELFMAG0 ||
        elf_header->e_ident[EI_MAG1] != ELFMAG1 ||
        elf_header->e_ident[EI_MAG2] != ELFMAG2 ||
        elf_header->e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "File is not a valid ELF file.\n");
        return -1;
    }

    // Ensure the ELF file is 32-bit
    if (elf_header->e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "File is not a 32-bit ELF file.\n");
        return -1;
    }

    // Get the program header table
    Elf32_Phdr *program_headers = (Elf32_Phdr *)((char *)map_start + elf_header->e_phoff);
    for (int i = 0; i < elf_header->e_phnum; i++) {
        func(&program_headers[i], arg);
    }

    return 0;
}

// Function to print program header information with mmap flags
void readelf_l(Elf32_Phdr *phdr, int index) {
    if (index == 0) {
        printf("Type       Offset   VirtAddr   PhysAddr   FileSiz  MemSiz  Flg Align   Prot  Map\n");
    }

    const char *type_name = get_phdr_type_name(phdr->p_type);
    const char *prot_flags = get_prot_flags(phdr->p_flags);
    int mmap_prot = calculate_mmap_prot(phdr->p_flags);

    printf("%-10s 0x%-7x 0x%-8x 0x%-8x 0x%-6x 0x%-6x %-3s 0x%-5x %s MAP_PRIVATE\n",
        type_name,
        phdr->p_offset,
        phdr->p_vaddr,
        phdr->p_paddr,
        phdr->p_filesz,
        phdr->p_memsz,
        get_prot_flags(phdr->p_flags),
        phdr->p_align,
        prot_flags);
}

void foo(){
    printf("the program is running hhhhhh");
}

int main(int argc, char **argv) {
    

    const char *file_name = argv[1];
    int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Error getting file size");
        close(fd);
        return 1;
    }

    void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return 1;
    }

    // Call the iterator function
    if (foreach_phdr(map_start, readelf_l, 0) < 0) {
        fprintf(stderr, "Error iterating over program headers.\n");
        munmap(map_start, st.st_size);
        close(fd);
        return 1;
    }

    if(foreach_phdr(map_start, load_phdr , fd)){
        fprintf(stderr, "Error iterating over program headers.\n");
        munmap(map_start, st.st_size);
        close(fd);
        return 1;
    }

    Elf32_Ehdr* elf_head = (Elf32_Ehdr*)map_start;

    int argcToPass = argc -1 ;
    char** argvToPass = argv + 1 ;
    printf("\n%d\n",argcToPass);
    printf("%s\n",argv[1]);
    foreach_phdr(map_start, readelf_l, 0);
    startup(argcToPass, argvToPass, (void *)(elf_head->e_entry));


    munmap(map_start, st.st_size);
    close(fd);
    return 0;
}

void load_phdr(Elf32_Phdr *phdr, int fd) {
    // Only map headers of type PT_LOAD
   if (phdr->p_type != PT_LOAD) {
        printf("Skipping non-loadable segment: %s\n", get_phdr_type_name(phdr->p_type));
        return;
    }

    if (phdr->p_memsz == 0) {
        printf("Skipping zero-sized segment: %s\n", get_phdr_type_name(phdr->p_type));
        return;
    }

    printf("%s\n", get_phdr_type_name(phdr->p_type));
    // Calculate the mmap protection flags
    int prot = 0;
    if (phdr->p_flags & PF_R) prot |= PROT_READ;
    if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
    if (phdr->p_flags & PF_X) prot |= PROT_EXEC;

    // Align the memory to the page size
    size_t offset_diff = phdr->p_offset & 0xfff;

    // Map enough memory to cover the segment size, aligned to page boundaries
    size_t map_size = phdr->p_memsz + offset_diff;

    // printf("hi1\n");

    // Map the memory using mmap
    void *mapped_addr = mmap((void *)(phdr->p_vaddr & 0xfffff000), // Align virtual address
                             map_size,
                             prot,
                             MAP_PRIVATE | MAP_FIXED,
                             fd,
                             phdr->p_offset & 0xfff000);


    // Check if mmap succeeded
    if (mapped_addr == MAP_FAILED) {
        perror("Error mapping program header");
        // printf("hi1\n");
        exit(1);
    }

    // Print information about the mapped program header
    printf("Mapped segment:\n");
    readelf_l(phdr, 0);
}
