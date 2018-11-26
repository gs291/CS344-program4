#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void genKey(int len) {
  int i;
  char key[len + 2];
  srand(time(NULL));

   memset(key , '\0', sizeof(key));

  for(i = 0; i < len; i++) {
    char c = (rand() % (91 + 1 - 65) + 65);
    if(c == '[') {
      c = 32;
    }
    key[i] = c;
  }
  key[len] = '\n';
  printf("%s", key);
}

int main (int argc, char *argv[]) {
  if(argc >= 2){
    int keyLength = atoi(argv[1]);
    if(keyLength > 0) {
      genKey(keyLength);
    }
    else {
      fprintf(stderr, "Invalid key length\n");
      exit(1);
    }
  } else {
    fprintf(stderr, "Insufficient arguments\n");
    exit(1);
  }
  return 0;
}
