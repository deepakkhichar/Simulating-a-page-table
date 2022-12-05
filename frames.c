#include <stdio.h>

#include <stdlib.h>

#include <string.h>

struct store {
  struct store * next;
  struct store * prev;
  int dirty;
  int loc;
  char * s;
  int indicator;
  struct node * roo;
  int future;
};

struct node {
  struct node * nex;
  int pos;
  struct node * tail;
};

struct store * root;
struct store * arr[2000000];
struct store * dup[2000000];
int access;
int misses;
int writes;
int drops;

char * convert_to_hex(int a) {
  char * hex_code;
  hex_code = malloc(8 * sizeof(char));
  int j = 6;
  while (a != 0) {
    int temp = a % 16;
    if (temp < 10)
      hex_code[j] = temp + '0';
    else
      hex_code[j] = temp + 'a' - 10;
    a = a / 16;
    j--;
  }
  for (int i = j; i >= 2; i--)
    hex_code[i] = '0';
  hex_code[0] = '0';
  hex_code[1] = 'x';
  hex_code[7] = '\0';
  return hex_code;
}

int power(int a, int b) {
  int ans = 1;
  for (int i = 0; i < b; i++)
    ans = ans * a;
  return ans;
}

int convert_hex_to_int(char * hex) {
  int deci = 0;
  int a = strlen(hex);
  a = a - 2;
  if (a <= 3)
    return 0;
  a = a + 1;
  int i = a - 3;
  while (i >= 2) {
    int temp = hex[i];
    if (temp >= 48 && temp <= 57)
      temp = temp - 48;
    else if (temp >= 97)
      temp = temp - 87;
    deci = deci + (temp * power(16, a - 3 - i));
    i--;
  }
  return deci;
}

void print_results() {
  printf("Number of memory accesses: %d\n", access);
  printf("Number of misses: %d\n", misses);
  printf("Number of writes: %d\n", writes);
  printf("Number of drops: %d\n", drops);
}

void print_verbose(char * read_page, char * old_page, int replace_type) {
  if (replace_type == 0)
    printf("Page %s was read from disk, page %s was written to the disk.\n", read_page, old_page);
  else
    printf("Page %s was read from disk, page %s was dropped (it was not dirty).\n", read_page, old_page);
}

void OPT(char * str, int frames, int vb) {
  FILE * fptr = fopen(str, "r");
  char in [50];
  while (fgets( in , sizeof( in ), fptr)) {
    access++;
    char * tkn[5];
    char * token;
    token = strtok( in , " \n");
    int pos = 0;
    while (token != NULL) {
      tkn[pos] = token;
      pos++;
      token = strtok(NULL, " \n");
    }
    char * ind = malloc(50);
    strncpy(ind, tkn[0], 7);
    int index = convert_hex_to_int(tkn[0]);
    if (dup[index] == NULL) {
      dup[index] = malloc(sizeof(struct store));
      dup[index] -> roo = malloc(sizeof(struct node));
      struct node * new = malloc(sizeof(struct node));
      new -> nex = NULL;
      new -> pos = access;
      dup[index] -> loc = index;
      dup[index] -> roo -> nex = new;
      dup[index] -> roo -> tail = new;
    } else {
      struct node * new = malloc(sizeof(struct node));
      new -> nex = NULL;
      new -> pos = access;
      dup[index] -> roo -> tail -> nex = new;
      dup[index] -> roo -> tail = new;
    }
  }
  fclose(fptr);

  struct store * RanArr[frames];
  root = malloc(sizeof(struct store));
  fptr = fopen(str, "r");
  struct store * temp = root;
  for (int i = 0; i < 2000000; i++) {
    if (dup[i] != NULL)
      dup[i] -> roo -> tail = dup[i] -> roo -> nex;
  }
  while (fgets( in , sizeof( in ), fptr)) {
    char * tkn[5];
    char * token;
    token = strtok( in , " \n");
    int pos = 0;
    while (token != NULL) {
      tkn[pos] = token;
      pos++;
      token = strtok(NULL, " \n");
    }
    char * ind = malloc(50);
    strncpy(ind, tkn[0], 7);
    int index = convert_hex_to_int(tkn[0]);
    if (arr[index] == NULL) {
      misses++;
      arr[index] = malloc(sizeof(struct store));
      if (strcmp(tkn[1], "R") == 0)
        arr[index] -> dirty = 0;
      else
        arr[index] -> dirty = 1;
      arr[index] -> loc = index;
      arr[index] -> s = strdup(ind);

      if (dup[index] -> roo -> tail -> nex != NULL) {
        arr[index] -> future = dup[index] -> roo -> tail -> nex -> pos;
        dup[index] -> roo -> tail = dup[index] -> roo -> tail -> nex;
      } else {
        arr[index] -> future = 100000001;
      }

      if (misses > frames) {
        int replace_type = 0;
        int farthest = 0;
        int evict;

        for (int i = 0; i < frames; i++) {
          if (RanArr[i] -> future > farthest) {
            farthest = RanArr[i] -> future;
            evict = i;
          }
        }
        if (RanArr[evict] -> dirty == 0) {
          drops++;
          replace_type = 1;
        } else
          writes++;

        if (vb == 1) {
          print_verbose(convert_to_hex(index), convert_to_hex(RanArr[evict] -> loc), replace_type);
        }
        int old_pag = RanArr[evict] -> loc;
        arr[index] -> next = RanArr[evict] -> next;
        arr[index] -> prev = RanArr[evict] -> prev;
        RanArr[evict] -> prev -> next = arr[index];
        if ((RanArr[evict] -> next) != NULL)
          RanArr[evict] -> next -> prev = arr[index];
        RanArr[evict] = arr[index];
        arr[old_pag] = NULL;
      } else {
        dup[index] -> future = 100000001;
        arr[index] -> next = NULL;
        arr[index] -> prev = temp;
        temp -> next = arr[index];
        RanArr[misses - 1] = arr[index];
        temp = arr[index];
      }
    } else {
      if (strcmp(tkn[1], "W") == 0)
        arr[index] -> dirty = 1;
      if (dup[index] -> roo -> tail -> nex != NULL) {
        arr[index] -> future = dup[index] -> roo -> tail -> nex -> pos;
        dup[index] -> roo -> tail = dup[index] -> roo -> tail -> nex;
      } else
        arr[index] -> future = 100000001;
    }
  }
  fclose(fptr);
}

void FIFO(char * str, int frames, int vb) {
  root = malloc(sizeof(struct store));
  FILE * fptr = fopen(str, "r");
  char in [50];
  struct store * temp = root;
  while (fgets( in , sizeof( in ), fptr)) {
    access++;
    char * tkn[5];
    char * token;
    token = strtok( in , " \n");
    int pos = 0;
    while (token != NULL) {
      tkn[pos] = token;
      pos++;
      token = strtok(NULL, " \n");
    }
    char * ind = malloc(50);
    strncpy(ind, tkn[0], 7);
    int index = convert_hex_to_int(tkn[0]);
    if (arr[index] == NULL) {
      misses++;
      if (misses > frames) {
        int replace_type = 0;
        if (root -> next -> dirty == 0) {
          drops++;
          replace_type = 1;
        } else
          writes++;

        if (vb == 1) {
          print_verbose(convert_to_hex(index), convert_to_hex(root -> next -> loc), replace_type);
        }
        int old_pag = root -> next -> loc;
        if (frames != 1)
          root -> next = root -> next -> next;
        else
          temp = root;
        arr[old_pag] = NULL;
      }

      arr[index] = malloc(sizeof(struct store));
      if (strcmp(tkn[1], "R") == 0)
        arr[index] -> dirty = 0;
      else
        arr[index] -> dirty = 1;
      arr[index] -> loc = index;
      arr[index] -> s = strdup(ind);
      arr[index] -> next = NULL;
      temp -> next = arr[index];
      temp = arr[index];
    } else {
      if (strcmp(tkn[1], "W") == 0)
        arr[index] -> dirty = 1;
    }
  }
  fclose(fptr);
}

void RANDOM(char * str, int frames, int vb) {
  struct store * RanArr[frames];
  root = malloc(sizeof(struct store));
  root -> prev = NULL;
  FILE * fptr = fopen(str, "r");
  char in [50];
  struct store * temp = root;
  srand(5635);
  while (fgets( in , sizeof( in ), fptr)) {
    access++;
    char * tkn[5];
    char * token;
    token = strtok( in , " \n");
    int pos = 0;
    while (token != NULL) {
      tkn[pos] = token;
      pos++;
      token = strtok(NULL, " \n");
    }
    char * ind = malloc(50);
    strncpy(ind, tkn[0], 7);
    int index = convert_hex_to_int(tkn[0]);
    if (arr[index] == NULL) {
      misses++;
      arr[index] = malloc(sizeof(struct store));
      if (strcmp(tkn[1], "R") == 0)
        arr[index] -> dirty = 0;
      else
        arr[index] -> dirty = 1;
      arr[index] -> loc = index;
      arr[index] -> s = strdup(ind);

      if (misses > frames) {
        int replace_type = 0;
        int evict = rand() % frames;
        if (RanArr[evict] -> dirty == 0) {
          drops++;
          replace_type = 1;
        } else
          writes++;

        if (vb == 1) {
          print_verbose(convert_to_hex(index), convert_to_hex(RanArr[evict] -> loc), replace_type);
        }
        int old_pag = RanArr[evict] -> loc;
        arr[index] -> next = RanArr[evict] -> next;
        arr[index] -> prev = RanArr[evict] -> prev;
        RanArr[evict] -> prev -> next = arr[index];
        if ((RanArr[evict] -> next) != NULL)
          RanArr[evict] -> next -> prev = arr[index];
        RanArr[evict] = arr[index];
        arr[old_pag] = NULL;
      } else {
        arr[index] -> next = NULL;
        arr[index] -> prev = temp;
        temp -> next = arr[index];
        RanArr[misses - 1] = arr[index];
        temp = arr[index];
      }
    } else {
      if (strcmp(tkn[1], "W") == 0)
        arr[index] -> dirty = 1;
    }
  }
  fclose(fptr);
}

void LRU(char * str, int frames, int vb) {
  root = malloc(sizeof(struct store));
  FILE * fptr = fopen(str, "r");

  char in [50];
  struct store * temp = root;
  while (fgets( in , sizeof( in ), fptr)) {
    access++;
    char * tkn[5];
    char * token;
    token = strtok( in , " \n");
    int pos = 0;
    while (token != NULL) {
      tkn[pos] = token;
      pos++;
      token = strtok(NULL, " \n");
    }
    char * ind = malloc(50);
    strncpy(ind, tkn[0], 7);
    int index = convert_hex_to_int(tkn[0]);
    if (arr[index] == NULL) {
      misses++;
      if (misses > frames) {
        int replace_type = 0;
        if (root -> next -> dirty == 0) {
          drops++;
          replace_type = 1;
        } else
          writes++;
        if (vb == 1) {
          print_verbose(convert_to_hex(index), convert_to_hex(root -> next -> loc), replace_type);
        }
        int old_pag = root -> next -> loc;
        if (frames != 1) {
          root -> next = root -> next -> next;
          root -> next -> prev = root;
        } else
          temp = root;

        arr[old_pag] = NULL;
      }

      arr[index] = malloc(sizeof(struct store));
      if (strcmp(tkn[1], "R") == 0)
        arr[index] -> dirty = 0;
      else
        arr[index] -> dirty = 1;
      arr[index] -> loc = index;
      arr[index] -> s = strdup(ind);
      arr[index] -> next = NULL;
      arr[index] -> prev = temp;
      temp -> next = arr[index];
      temp = arr[index];
    } else {
      if (strcmp(tkn[1], "W") == 0)
        arr[index] -> dirty = 1;
      if (temp != arr[index]) {
        arr[index] -> prev -> next = arr[index] -> next;
        if (arr[index] -> next != NULL)
          arr[index] -> next -> prev = arr[index] -> prev;
        temp -> next = arr[index];
        arr[index] -> next = NULL;
        arr[index] -> prev = temp;
        temp = arr[index];
      }
    }
  }
  fclose(fptr);
}

void CLOCK(char * str, int frames, int vb) {
  struct store * ClockArr[frames];
  int p = 0;
  root = malloc(sizeof(struct store));
  FILE * fptr = fopen(str, "r");
  char in [50];
  struct store * temp = root;
  while (fgets( in , sizeof( in ), fptr)) {
    access++;
    char * tkn[5];
    char * token;
    token = strtok( in , " \n");
    int pos = 0;
    while (token != NULL) {
      tkn[pos] = token;
      pos++;
      token = strtok(NULL, " \n");
    }
    char * ind = malloc(50);
    strncpy(ind, tkn[0], 7);
    int index = convert_hex_to_int(tkn[0]);
    if (arr[index] == NULL) {
      misses++;
      arr[index] = malloc(sizeof(struct store));
      if (strcmp(tkn[1], "R") == 0)
        arr[index] -> dirty = 0;
      else
        arr[index] -> dirty = 1;
      arr[index] -> loc = index;
      arr[index] -> s = strdup(ind);
      arr[index] -> indicator = 1;
      if (misses > frames) {
        int replace_type = 0;
        int loop = 0;
        while (ClockArr[p] -> indicator == 1 && loop < frames) {
          ClockArr[p] -> indicator = 0;
          p = (p + 1) % frames;
          loop++;
        }
        if (ClockArr[p] -> dirty == 0) {
          drops++;
          replace_type = 1;
        } else
          writes++;

        if (vb == 1) {
          print_verbose(convert_to_hex(index), convert_to_hex(ClockArr[p] -> loc), replace_type);
        }
        int old_pag = ClockArr[p] -> loc;
        arr[index] -> next = ClockArr[p] -> next;
        arr[index] -> prev = ClockArr[p] -> prev;
        ClockArr[p] -> prev -> next = arr[index];
        if ((ClockArr[p] -> next) != NULL)
          ClockArr[p] -> next -> prev = arr[index];
        ClockArr[p] = arr[index];
        arr[old_pag] = NULL;
        p = (p + 1) % frames;
      } else {
        arr[index] -> next = NULL;
        arr[index] -> prev = temp;
        temp -> next = arr[index];
        ClockArr[misses - 1] = arr[index];
        temp = arr[index];
      }
    } else {
      if (strcmp(tkn[1], "W") == 0)
        arr[index] -> dirty = 1;
      arr[index] -> indicator = 1;
    }
  }
  fclose(fptr);
}

int main(int argc, char ** argv) {
  access = 0;
  misses = 0;
  writes = 0;
  drops = 0;
  int frames = atoi(argv[2]);
  int vb = 0;
  if (argc == 5 && strcmp(argv[4], "-verbose") == 0)
    vb = 1;

  if (strcmp(argv[3], "OPT") == 0) {
    OPT(argv[1], frames, vb);
    print_results();
  } else if (strcmp(argv[3], "FIFO") == 0) {
    FIFO(argv[1], frames, vb);
    print_results();
  } else if (strcmp(argv[3], "CLOCK") == 0) {
    CLOCK(argv[1], frames, vb);
    print_results();
  } else if (strcmp(argv[3], "LRU") == 0) {
    LRU(argv[1], frames, vb);
    print_results();
  } else if (strcmp(argv[3], "RANDOM") == 0) {
    RANDOM(argv[1], frames, vb);
    print_results();
  }

  return 0;
}