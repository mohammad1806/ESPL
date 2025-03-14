#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 
char* map(char *array, int array_length, char (*f) (char)){
  char* mapped_array = (char*)(malloc(array_length*sizeof(char)));
  /* TODO: Complete during task 2.a */

  
    if (mapped_array == NULL) {
        printf("Memory allocation failed\n");
        exit(1); 
    }

    
    for (int i = 0; i < array_length; i++) {
        mapped_array[i] = f(array[i]);
    }

  return mapped_array;
}

char my_get(char c)
{
  return fgetc(stdin);
}

char cprt(char c)
{
  if ((c > 0X20) && (c < 0X7E))
    printf("%c\n", c);
  else
    printf(".\n");
  return c;
}

char encrypt(char c)
{
  if ((c > 0X1F) && (c < 0X7E))
    return c + 1;
  return c;
}

char decrypt(char c)
{
  if ((c > 0X21) && (c < 0X7F))
    return c - 1;
  return c;
}

char xprt(char c)
{
  printf("%x\n", c);
  return c;
}

char dprt(char c) {
    printf("%d\n", (char)c); // Print the decimal representation
    return c;
}

int main(int argc, char **argv){
  /* TODO: Test your code */

  int base_len = 5;
  char arr1[base_len];
  char* arr2 = map(arr1, base_len, my_get);
  char* arr3 = map(arr2, base_len, cprt);
  //map(arr2,base_len,cprt);
  char* arr4 = map(arr3, base_len, xprt);
  char* arr5 = map(arr4, base_len, encrypt);
  free(arr2);
  free(arr3);
  free(arr4);
  free(arr5);

// char arr1[] = {'H','E','Y','!'};
// char* arr2 = map(arr1, 4, xprt);
// printf("%s\n", arr2);
// free(arr2);

return 0;

}