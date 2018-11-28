#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Name: generate key
 * Description: This generates a random string of a specific size of characters
 *    'A'-'Z' including the space (' ') character
 */
void genKey(int len) {
  int i;
  char key[len + 2]; //account for the '\n' and '\0'
  srand(time(NULL));

  memset(key , '\0', sizeof(key)); //Clear out the key string

  for(i = 0; i < len; i++) {
    //Create a random char ('A'-']') using ascii values
    char c = (rand() % (91 + 1 - 65) + 65);
    if(c == '[') { c = 32; } //If the character should be a space
    key[i] = c;
  }
  key[len] = '\n';
  printf("%s", key); //Output the key to stdout
}

int main (int argc, char *argv[]) {
  if(argc >= 2){ // Check for the correct amount of arguments
    int keyLength = atoi(argv[1]); //Convert the second argument to an int
    if(keyLength > 0) { //Check for a valid input
      genKey(keyLength);
    }
    else { //print error for a random second argument
      fprintf(stderr, "Invalid key length\n");
      exit(1);
    }
  } else { // print error for only calling keygen
    fprintf(stderr, "Insufficient arguments\n");
    exit(1);
  }
  return 0;
}
