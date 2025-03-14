#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 16 // Number of bytes to read at a time

// Helper function to print bytes in hexadecimal format
void PrintHex(const unsigned char *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", buffer[i]);
    }
}

int main(int argc, char *argv[]) {
    // Ensure the program is called with one argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <binary file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Open the binary file for reading
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    unsigned char buffer[BUFFER_SIZE];
    size_t bytesRead;

    // Read the file in chunks of BUFFER_SIZE bytes
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        // Use the helper function to print the bytes in hexadecimal format
        PrintHex(buffer, bytesRead);
    }
    printf("\n");

    if (ferror(file)) {
        perror("Error reading file");
        fclose(file);
        return EXIT_FAILURE;
    }

    // Close the file
    fclose(file);

    return EXIT_SUCCESS;
}
