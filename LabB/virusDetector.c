#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct virus
{
    unsigned short SigSize;
    char virusName[16];
    unsigned char *signature;
} virus;

typedef struct link link;
struct link
{
    link *nextVirus;
    virus *vir;
};

struct fun_desc
{
    char *name;
    void (*fun)();
};

link *list = NULL;
char filebuffer[10000];

virus *readVirus(FILE *file, int big)
{
    virus *v = malloc(sizeof(virus));
    int isB = big;
    char a[2];
    if (isB == 1)
    {
        if (fread(a, sizeof(unsigned short), 1, file) != 1)
        {
            free(v);
            return 0;
        }
        v->SigSize = ((unsigned char *)a)[0] * 16 * 16 + ((unsigned char *)a)[1];
    }
    else
    {
        if (fread(&(v->SigSize), sizeof(unsigned short), 1, file) != 1)
        {
            free(v);
            return 0;
        }
    }
    if (fread(v->virusName, sizeof(char), 16, file) != 16)
    {
        free(v);
        return 0;
    }

    v->signature = malloc(v->SigSize);

    if (fread(v->signature, sizeof(unsigned char), v->SigSize, file) != v->SigSize)
    {
        free(v->signature);
        free(v);
        return 0;
    }

    return v;
}
void printVirus(virus *v, FILE *output) {
    
    fprintf(output, "Virus Name in (ASCII): ");
    for (int i = 0; i < 16 && v->virusName[i] != '\0'; i++) {
        fprintf(output, "%d ", (unsigned char)v->virusName[i]);
    }
    fprintf(output, "\n");

    
    fprintf(output, "Signature Size: %u\n", v->SigSize);

    
    fprintf(output, "Signature:\n");
    for (unsigned short i = 0; i < v->SigSize; ++i) {
        fprintf(output, "%02X ", v->signature[i]);
    }

    
    fprintf(output, "\n");
}

void list_print(link *virus_list, FILE *out)
{
    link *curr = virus_list;
    while (curr != NULL)
    {
        printVirus(curr->vir, out);
        curr = curr->nextVirus;
    }
}

link *list_append(link *virus_list, virus *data)
{
    link *newLink = (link *)malloc(sizeof(link));
    newLink->vir = data;
    if (virus_list != NULL)
    {
        newLink->nextVirus = virus_list;
    }
    return newLink;
}

void list_free(link *virus_list)
{
    link *curr = virus_list;
    while (curr != NULL)
    {
        link *next = curr->nextVirus;
        free(curr->vir);
        free(curr);
        curr = next;
    }
}
void Load_signatures()
{
    char file[30];
    scanf("%29s", file);
    int isB = 0;
    FILE *signaturesFile = fopen(file, "rb");

    if (!signaturesFile)
    {
        fprintf(stderr, "Failed to open file '%s'\n", file);
        return;
    }
    char magicNumber[4];
    if (fread(magicNumber, sizeof(char), (unsigned int)4, signaturesFile) != 4)
    {
        printf("Error reading magic number from signatures file\n");
        fclose(signaturesFile);
        return;
    }
    char tempBuffer[4];
    memcpy(tempBuffer, magicNumber, 4);
    tempBuffer[4] = '\0';
    if (strcmp(tempBuffer, "VIRB") == 0)
    {
        isB=1;
    }
    while (1)
    {
        virus *v = readVirus(signaturesFile, isB);
        if (!v)
        {
            break;
        }
        list = list_append(list, v);
    }
    fclose(signaturesFile);
}
void Print_signatures()
{
    if (list != NULL)
    {
        list_print(list, stdout);
    }
}
void detect_virus(char *buffer, unsigned int size, link *virus_list)
{
    while (virus_list != NULL)
    {
        for (int i = 0; i <= size - virus_list->vir->SigSize; i++)
        {
            if (memcmp(buffer + i, virus_list->vir->signature, virus_list->vir->SigSize) == 0)
            {
                fprintf(stderr, "Starting byte: %d\n", i);
                fprintf(stderr, "Virus name: %s\n", virus_list->vir->virusName);
                fprintf(stderr, "Signature size: %d\n", virus_list->vir->SigSize);
            }
        }
        virus_list = virus_list->nextVirus;
    }
}

int filesize(FILE *file)
{
    int size = 0;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}
void Detect_viruses(FILE *file)
{
    int size = filesize(file);
    fread(filebuffer, 1, size, file);
    detect_virus(filebuffer, size, list);
}
int indexOfVirus(FILE* file){
    int size=filesize(file);
    fread(filebuffer, 1, size, file);
    while(list!=NULL){
        for(int i=0;i<size-list->vir->SigSize;i++){
            if(memcmp(filebuffer+i,list->vir->signature,list->vir->SigSize)==0){
                return i;
            }
        }
        list=list->nextVirus;
    }
    return -1;
}

void neutralize_virus(char *fileName, int signatureOffset){
    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        printf("Failed to open file.\n");
        return;
    }
    fseek(file, signatureOffset, SEEK_SET);
    unsigned char retInstruction = 0xC3;
    fwrite(&retInstruction, sizeof(unsigned char), 1, file);
    fclose(file);
    printf("Virus in byte %d is cleand.\n",signatureOffset);
}
void Fix_file(char* filename)
{
    FILE* file=fopen(filename,"rb");
    int index;
    while(indexOfVirus(file) != -1){
        index= (int) indexOfVirus(file);
        neutralize_virus(filename,index);
    }
}
void Quit()
{
    if (list != NULL)
    {
        list_free(list);
    }
    exit(0);
}

struct fun_desc menu[] = {
    {"Load signatures", Load_signatures},
    {"Print signatures", Print_signatures},
    {"Detect viruses", Detect_viruses},
    {"Fix File", Fix_file},
    {"Quit", Quit},
    {NULL, NULL}
};

int main(int argc, char **argv)
{
    char inputBuffer[50];
    int selection = -1;
    const size_t menuSize = sizeof(menu) / sizeof(menu[0]);

    while (1)
    {
        printf("Please choose an option:\n");
        for (int i = 0; i < menuSize - 1; i++)
        {
            printf("%d) %s\n", i, menu[i].name);
        }

        scanf("%s", inputBuffer);
        selection = inputBuffer[0] - '0';

        switch (selection)
        {
            case 0:
                printf("Type file name: ");
                Load_signatures();
                break;
            case 1:
                Print_signatures();
                break;
            case 2:
                {
                    FILE *file = fopen(argv[1], "r");
                    if (file == NULL)
                    {
                        fprintf(stderr, "Error opening file\n");
                        exit(1);
                    }
                    Detect_viruses(file);
                    fclose(file);
                }
                break;
            case 3:
                Fix_file(argv[1]);
                break;
            case 4:
                Quit();
                break;
            default:
                printf("Invalid selection\n");
                break;
        }
    }

    return 0;
}
