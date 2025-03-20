#include <stdio.h>

int count_digits(const char *str) {
    int digit_count = 0;  // Variable to count digits


    // Loop through each character in the string
    while (*str != '\0') {
        // Check if the character is a digit (between '0' and '9')
        if (*str >= '0' && *str <= '9') { 
            digit_count++;  // Increment the digit count
        }

        // Move to the next character in the string
        str++;

    }

    return digit_count;  // Return the count of digits
}

int main(){
    return 0;
}