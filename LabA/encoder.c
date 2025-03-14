#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

char encode(char c, int inc, int sign)
{
    if (sign == -1)
    {
        inc = inc * -1;
    }
    if (c >= 'A' && c <= 'Z')
    {
        return (char)(((c - 'A' + inc) % 26 + 26) % 26 + 'A');
    }
    else if (c >= 'a' && c <= 'z'){

        return (char)(((c - 'a' + inc) % 26 + 26) % 26 + 'a');
    }
    else if (c >= '0' && c <= '9')
    {
        return (char)(((c - '0' + inc) % 10 + 10) % 10 + '0');
    }
    else
    {
        return c;
    }
}

int main(int argc, char **argv)
{
    FILE *input = stdin;
    FILE *output = stdout;
    int flag = 1;
    int sign = 0;
    char *numberStr;
    char curr;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-D") == 0)
        {
            flag = 0;
            fprintf(stderr, "Arguments : %s\n", argv[i]);

        }
        else if (strcmp(argv[i], "+D") == 0)
        {
            flag = 1;
        }
        else
        {
            if (flag == 1)
            {
                fprintf(stderr, "Arguments : %s\n", argv[i]);
            }
        }

        if ((strncmp(argv[i], "+E", 2) == 0))
        {
            sign = 1;
            numberStr = argv[i] + 2;
        }
        else if ((strncmp(argv[i], "-E", 2) == 0))
        {
            sign = -1;
            numberStr = argv[i] + 2;
        }
        if (strncmp(argv[i], "-i", 2) == 0)
        {
            input = fopen(argv[i] + 2, "r");
            if (!input)
            {
                fprintf(stderr, "Error opening file (file not exist) \n");
                exit(0);
            }
        }
        if (strncmp(argv[i], "-o", 2) == 0)
        {
            output = fopen(argv[i] + 2, "w");
            if (!output)
            {
                fprintf(stderr, "Error opening file (file not exist) \n");
                exit(0);
            }
        }
    }

    int str_len = 0;
    while (numberStr[str_len] != '\0') {
        str_len++;
    }

    int d = 0;
    while ((curr = fgetc(input)) != EOF)
    {
        int inc = numberStr[d % str_len] - '0';
        char shifted = encode(curr, inc, sign);
        d++;
        fputc(shifted, output);
    }
    printf("\n");
    fclose(input);
    fclose(output);
    return 0;
}