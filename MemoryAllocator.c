#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <math.h>

// Memory Allocator in C


typedef struct Heap_Byte {
  unsigned char data;
  bool isHeader;
  bool isFooter; //iSfooter is true and size_block is -1, then its part of allocated block
//isFooter is true and size block has data, then its a part of a free block
  int payload_size; // payload size
  bool isAllocated; 
} HB; 


void getUserArgs(char * user_input, char user_args[3][80]);
void initializeStructs(HB * byte_list);
unsigned char createHeader(unsigned char size, bool allocated);
void translateData(unsigned char header, bool * allocated, int * sizeBlock);

void processCommand(HB * byte_list, int argc, char * argv[], char user_args[3][80]);
// processCommand helper functions
void firstfit_malloc(HB * byte_list, char * user_size_arg);
void bestfit_malloc(HB * byte_list, char * user_size_arg);
void freeCommand(HB * byte_list, char user_args[3][80]);
void blocklistCommand(HB * byte_list);
void writememCommand(HB * byte_list, char user_args[3][80]);
void printmemCommand(HB * byte_list, char user_args[3][80]);

int first_available = 0;


int main(int argc, char * argv[]) {
  HB byte_list[127];
  initializeStructs(byte_list);
  
  char user_in[80] = {0};
  while (!(strstr(user_in, "quit") == user_in)) {
    // reset args and fields
    user_in[0] = '\0';
    char user_args[3][80] = {0, 0, 0};
    printf(">");
    char * result = fgets(user_in, sizeof user_in, stdin); // get command, get arguments
    getUserArgs(user_in, user_args); // tokenize user argument
    
    processCommand(byte_list, argc, argv, user_args);
    }
}



void initializeStructs(HB * byte_list) {
  HB first_byte;
  first_byte.payload_size = 125;
  first_byte.isAllocated = false;
  first_byte.isHeader = true;
  first_byte.data = createHeader(first_byte.payload_size, first_byte.isAllocated);

  first_byte.isFooter = false;
  byte_list[0] = first_byte;
  
  for (int i = 1; i < 126; ++i) { //1-125, exclude first and last
    HB new_byte;
    new_byte.isAllocated = false;
    new_byte.data = 0; //starts 0; empty
    new_byte.isHeader = false;
    new_byte.isFooter = false;
    byte_list[i] = new_byte;
  }

  HB last_byte;
  last_byte.data = 0; //is this necessary? will this ever be read?
  last_byte.payload_size = 125; //size block only valid if isHeader
  last_byte.isAllocated = false;
  last_byte.isHeader = false;
  last_byte.isFooter = true;
  byte_list[126] = last_byte; //last, 127th byte
  
}


void processCommand(HB * byte_list, int argc, char * argv[], char user_args[3][80]) {
  extern int first_available;
  // user_args[0] = malloc, free, blocklist, writemem, printmem
  // user_args[1] = int size, int index, 
  // user_args[2] = char * str, int number_of_charcters_to_print
  
  /* malloc <int size> */
  char * malloc_mode_changeback = "FirstFit";


  // printf("DEBUG: ARGC: %d\n", argc);
  // printf("DEBUG: ARGV[0]: %s\n", argv[0]);
  // printf("DEBUG: ARGV[1]: %s\n", argv[1]);


  char malloc_mode[9];
  if (argc >= 2) {
    //char malloc_mode[strlen(argv[1])];
    strcpy(malloc_mode, argv[1]);
  } else {
    // char malloc_mode[8];
    strcpy(malloc_mode, "FirstFit");
  }

  // if (argc > 1 && strcmp(argv[1], "BestFit") == 0) {
  //   printf("DEBUG: SEGFAULT 0\n");
  //   strcpy(malloc_mode, "BestFit");
  // }
  
  if(argc == 1 && strcmp(user_args[0], "malloc") == 0) {
    firstfit_malloc(byte_list, user_args[1]);
    return;
  } 

  if (strcmp(user_args[0], "malloc") == 0 && strcmp(malloc_mode, "FirstFit")==0) {
    firstfit_malloc(byte_list, user_args[1]);
    return;
  } else if (strcmp(user_args[0], "malloc") == 0 && strcmp(malloc_mode, "BestFit")==0) {
    bestfit_malloc(byte_list, user_args[1]);

    return;
  }
  
  /* free <int index> */
  if (strcmp(user_args[0], "free") == 0) {
    freeCommand(byte_list, user_args);
  }
  
  /* blocklist */
  if (strcmp(user_args[0], "blocklist\n") == 0) {
    blocklistCommand(byte_list);
  }
  
  /* writemem <int index>, <char * str> */
  if (strcmp(user_args[0], "writemem") == 0) {
    // printf("DEBUG: writemem...\n");
    writememCommand(byte_list, user_args);
  }
  
  /* printmem <int index>, <int number_of_charcters_to_print> */
  if (strcmp(user_args[0], "printmem") == 0) {
    // printf("DEBUG: printmem...\n");
    printmemCommand(byte_list, user_args);
  }
  
}


void freeCommand(HB * byte_list, char user_args[3][80]) {
  extern int first_available;
  //assuming it exists
  int payload_index = atoi(user_args[1]);
  int current_header_index = payload_index-1; //this is the CURRENT HEADER
  int footer_index = current_header_index + byte_list[current_header_index].payload_size + 1;
  bool is_prev_alloc;
  bool is_next_alloc;
  if (current_header_index <= 0) {
    is_prev_alloc = true; //if index is 0 and prev_alloc is true prevents overlapping cases
    is_next_alloc = byte_list[footer_index+1].isAllocated;
  } else {
    is_prev_alloc = (byte_list[current_header_index-1].isFooter && (byte_list[current_header_index-1].payload_size == -1));
    is_next_alloc = byte_list[footer_index+1].isAllocated;
    
  }
  
  //CASE 2: PREV IS ALLOC (OR DOES NOT EXIST), NEXT IS FREE
  //if beginning and next block is free OR if previous is alloc and next is free
  if (is_prev_alloc && !is_next_alloc) {
    // printf("DEBUG: CASE 2 RUNNING!\n");
    //get size info
    int next_size;
    int new_payload_size;
    bool isAllocated;
    if (footer_index < 123) {
    translateData(byte_list[footer_index+1].data, &isAllocated, &next_size);
    new_payload_size = byte_list[current_header_index].payload_size+1 + 1+next_size;
    } else { //123, 124, 125
      isAllocated = true;
      next_size = 0;
      //payload size has already been set in malloc
      new_payload_size = byte_list[current_header_index].payload_size;
      
    }
    
    
    if (footer_index < 126) {
      //free current footer
      //isheader already set correctly
      byte_list[footer_index].isFooter = false;
      byte_list[footer_index].isAllocated = false;
      byte_list[footer_index].data = 0;
      byte_list[footer_index].payload_size = -1;
      //free next header
      //isfooter already set correctly
      byte_list[footer_index+1].isHeader = false;
      byte_list[footer_index+1].isAllocated = false;
      byte_list[footer_index+1].data = 0;
      byte_list[footer_index+1].payload_size = -1;
      
      //set next footer, which is at the end of the second block
      int new_footer_index = footer_index+1+next_size+1;
      byte_list[new_footer_index].isAllocated = false;
      byte_list[new_footer_index].data = createHeader(new_payload_size, false);
      byte_list[new_footer_index].isFooter = true;
      byte_list[new_footer_index].isHeader = false;
      byte_list[new_footer_index].payload_size = new_payload_size;
    } else { //>= 126 case
      //set current footer to free mode
      
      byte_list[footer_index].isFooter = true;
      byte_list[footer_index].isAllocated = false;
      byte_list[footer_index].data = createHeader(new_payload_size, true);
      byte_list[footer_index].payload_size = new_payload_size;
    }
    
    
    

    //set header after coalescing
    //isfooter and isheader already set correctly
    byte_list[current_header_index].isAllocated = false;
    byte_list[current_header_index].data = createHeader(new_payload_size, false);
    byte_list[current_header_index].payload_size = new_payload_size;

    //free body
    for (int i = current_header_index+1; i < footer_index; ++i) {
      byte_list[i].data = 0;
    }
    
    //first available is set to: header of CURR block IF smaller than current first_a
    if (current_header_index < first_available) {
      first_available = current_header_index;
    }
    return;
  }
  
  // CASE 1: PREV IS ALLOC, NEXT IS ALLOCATED
  //if beginning and next block is alloc OR if previous is alloc and next is alloc
  if (is_prev_alloc && is_next_alloc) {
    
    // printf("DEBUG: CASE 1 RUNNING!\n");
    //isheader, isfooter, size_block already set
    //free current header
    byte_list[current_header_index].isAllocated = false; 
    byte_list[current_header_index].data = createHeader(byte_list[current_header_index].payload_size, false);
  
    //set current footer to free
    //isfooter, isheader already set correctly
    byte_list[footer_index].isAllocated = false;
    byte_list[footer_index].data = createHeader(byte_list[current_header_index].payload_size, false);
    byte_list[footer_index].payload_size = byte_list[current_header_index].payload_size;

    //free body
    for (int i = current_header_index+1; i < footer_index; ++i) {
      byte_list[i].data = 0;
    }
    //first available is set to: header of CURR block IF smaller than current first_a
    if (current_header_index < first_available) {
      first_available = current_header_index;
    }
    return;
  }
  
  // CASE 3: PREV IS FREE, NEXT IS ALLOCATED
  if (!is_prev_alloc && is_next_alloc) {
    // printf("DEBUG: CASE 3 RUNNING\n");
    byte_list[current_header_index].isAllocated = false; //free it
    byte_list[current_header_index].data = createHeader(byte_list[current_header_index].payload_size, false);

    int prev_size;
    bool isAllocated;
    translateData(byte_list[current_header_index-1].data, &isAllocated, &prev_size);

    int new_payload_size = prev_size+1 + 1+byte_list[current_header_index].payload_size; //free'd but still has size info
    
    //free current header 
    byte_list[current_header_index].isHeader = false;
    byte_list[current_header_index].isAllocated = false;
    byte_list[current_header_index].data = 0;
    byte_list[current_header_index].payload_size = 0;
    //free previous footer
    byte_list[current_header_index-1].isFooter = false;
    byte_list[current_header_index-1].isAllocated = false;
    byte_list[current_header_index-1].data = 0;
    byte_list[current_header_index-1].payload_size = 0;

    // set prev header
    int prev_header_index = current_header_index-1-prev_size-1;
    byte_list[prev_header_index].data = createHeader(new_payload_size, false);
    byte_list[prev_header_index].payload_size = new_payload_size;

    //set current footer to free
    byte_list[footer_index].isAllocated = false;
    byte_list[footer_index].data = createHeader(new_payload_size, false);
    byte_list[footer_index].payload_size = new_payload_size;

    //free body
    for (int i = current_header_index+1; i < footer_index; ++i) {
      byte_list[i].data = 0;
    }
    
    //first available is set to: header of the PREV block IF smaller than current first_a
    if (prev_header_index < first_available) {
      first_available = prev_header_index;
    }
    return;
  }
  // CASE 4: PREV IS FREE, NEXT IS FREE
  if (!is_prev_alloc && !is_next_alloc) {
    // printf("DEBUG: CASE 4 RUNNING\n");

    //get all size info
    int curr_size = byte_list[current_header_index].payload_size;
    int prev_size;
    int next_size;
    bool isAllocated;
    translateData(byte_list[current_header_index-1].data, &isAllocated, &prev_size); //prev size data
    translateData(byte_list[footer_index+1].data, &isAllocated, &next_size); //next size data
    
    int new_payload_size = prev_size+1 + 1+curr_size+1 + 1+next_size; //free'd but still has size info
      
    // free prev footer
    byte_list[current_header_index-1].isFooter = false;
    byte_list[current_header_index-1].isAllocated = false;
    byte_list[current_header_index-1].data = 0;
    byte_list[current_header_index-1].payload_size = 0;
    // free current header
    byte_list[current_header_index].isHeader = false;
    byte_list[current_header_index].isAllocated = false;
    byte_list[current_header_index].data = 0;
    byte_list[current_header_index].payload_size = 0;
    // free current footer
    byte_list[footer_index].isFooter = false;
    byte_list[footer_index].isAllocated = false;
    byte_list[footer_index].data = 0;
    byte_list[footer_index].payload_size = 0;
    // free next header
    byte_list[footer_index+1].isHeader = false;
    byte_list[footer_index+1].isAllocated = false;
    byte_list[footer_index+1].data = 0;
    byte_list[footer_index+1].payload_size = 0;

    // set prev header
    int prev_header_index = current_header_index-1-prev_size-1;

    
    //isFooter and isHeader already set
    byte_list[prev_header_index].data = createHeader(new_payload_size, false);
    byte_list[prev_header_index].payload_size = new_payload_size;
    byte_list[prev_header_index].isAllocated = false;
    
    // set next footer
    int new_footer_index = footer_index+1+next_size+1;
    byte_list[new_footer_index].isAllocated = false;
    byte_list[new_footer_index].data = createHeader(new_payload_size, false);
    byte_list[new_footer_index].isFooter = true;
    byte_list[new_footer_index].isHeader = false;
    byte_list[new_footer_index].payload_size = new_payload_size;
    
    //free body
    for (int i = current_header_index+1; i < footer_index; ++i) {
      byte_list[i].data = 0;
    }
    
    //first available is set to: header of the PREV block IF smaller than current first_a
    if (prev_header_index < first_available) {
      first_available = prev_header_index;
    }
    return;
  }
}


void blocklistCommand(HB * byte_list) {
  // start with 125
  char sorted_blocks[127][100] = {""}; // array for sorting blocks
  char block_string[100]; 
  char block_size_string[100]; // temp string to convert int block_size to str
  char payload_string[100]; // temp string to convert int payload to str

  int payload_start = 0;
  int block_counter = 0;


  for (int b = 0; b < 127; b++) {
    if (byte_list[b].isHeader) { // header has been found
      payload_start = b + 1;
   
      // concatenate block info & added to sorted_data
      sprintf(block_size_string, "%d", byte_list[b].payload_size); // put int block_size into char block_size_string
      strcpy(block_string, block_size_string); // copy block_size string to block_string
      strcat(block_string, "-"); // add dash
      sprintf(payload_string, "%d", payload_start); // put int payload_start into char payload_string
      strcat(block_string, payload_string); // concatenate to block_size-payload
      strcat(block_string, "-"); // add dash
      
      if (byte_list[b].isAllocated) {
        strcat(block_string, "allocated");
      } else {
        strcat(block_string, "free");
      }

      strcpy(sorted_blocks[block_counter], block_string); // current block added to sorted_blocks
    
      // reset block information & increment block counter
      block_counter++;
      payload_start = 0;

      char block_string[100] = {0}; 
      char block_size_string[100] = {0}; // temp string to convert int block_size to str
      char payload_string[100] = {0}; // temp string to convert int payload to str     
    }
  }


  
  // sort the array 
  for (int i = 0; i < block_counter; i++) {
    for (int j = i + 1; j < block_counter; j++) {
      char temp_string1[100];
      char temp_string2[100];
      strcpy(temp_string1, sorted_blocks[i]);
      strcpy(temp_string2, sorted_blocks[j]);
      
      char *cblock_size_i = strtok(temp_string1, "-");
      
      char *cblock_size_j = strtok(temp_string2, "-");

      int block_size_i = atoi(cblock_size_i); 
      int block_size_j = atoi(cblock_size_j);
      
      if (block_size_i < block_size_j) {
        char temp[100] = {0};
        strcpy(temp, sorted_blocks[i]);
        strcpy(sorted_blocks[i], sorted_blocks[j]);
        strcpy(sorted_blocks[j], temp);
      }
    }
  }

  // print sorted array
  for (int k = 0; k < 127; k++) {
    if (strcmp(sorted_blocks[k], "") != 0)
      printf("%s\n", sorted_blocks[k]); //necessary for output
  }
  return;
}

void writememCommand(HB * byte_list, char user_args[3][80]) {
  int index = atoi(user_args[1]);
  int test_num = atoi(user_args[1]);
  char str[strlen(user_args[2])];
  strcpy(str, user_args[2]);

  // writes str character by character into byte_list
  for (int c = 0; c < strlen(user_args[2]); c++) { 
    byte_list[index].data = str[c];
    index++;
  }

  // test if writemem is writing correctly
  // for (int i = 0; i < strlen(user_args[2]); i++) {
  //   printf("byte_list[%d].data = %d\n", test_num, byte_list[test_num].data);
  //   test_num++;
  // }
}

void printmemCommand(HB * byte_list, char user_args[3][80]) {
  int index = atoi(user_args[1]);
  int num_characters_print = atoi(user_args[2]);


  
  for (int c = 0; c < num_characters_print; c++) {
    
    printf("%d", byte_list[index].data); //necessary for output
    if (c == (num_characters_print - 1)) {
      break;
    }
    printf("-");
    index++;
  }
  printf("\n");
}



void getUserArgs(char * user_input, char user_args[3][80]) {
  //returns everything null terminated
  char * token = strtok(user_input, " ");
  int i = 0;
  //null terminate user_input
  user_input[strlen(user_input)] = '\0'; //strlen includes \n
  while (token != NULL) {
    strcpy(user_args[i], token);
    token = strtok(NULL, " "); //increment
    ++i;
  }
  return;
}

unsigned char createHeader(unsigned char size, bool allocated) {
  unsigned char header = 0;
  header = size << 1;
  header = header + allocated; //adds 0 if false (free), adds 1 if true (allocated)
  return header;
}

void translateData(unsigned char header, bool * allocated, int * sizeBlock) {
  //bitwise & gets lsb, then != 0 converts to bool
  *allocated = ((0xF & header) != 0); //0 for free, 1 for allocated
  *sizeBlock = header >> 1; //shift right 1 to only leave 7 msb
  return;
}

void firstfit_malloc(HB * byte_list, char * user_arg_size) {


  
  extern int first_available;
  //true for allocated
  int user_size = atoi(user_arg_size);
  
  // subtract the size of the free block to be filled by the size of the block the user is attempting to malloc
  int gap_size = (1+byte_list[first_available].payload_size+1) - (1+user_size+1);
  if (gap_size < 3) {
    user_size = user_size + gap_size;
  }
  //MAKE HEADER BYTE
  HB new_header;
  new_header.isAllocated = true;
  new_header.data = createHeader(user_size, new_header.isAllocated);
  new_header.isFooter = false;
  new_header.isHeader = true;
  new_header.payload_size = user_size;
  byte_list[first_available] = new_header;
  if (first_available >= 126) {
    // printf("DEBUG: HEAP FILLED!\n");
    return;
  }
  //PAYLOAD SPOTS SET TO 0
  //eg 0+1 = 1, i < 0 + 10 + 1 == 1-10 (inc)
  for (int i = first_available+1; i < first_available + user_size+1; ++i) {
    byte_list[i].data = 0;
    byte_list[i].isAllocated = true;
    byte_list[i].isHeader = false;
    byte_list[i].isFooter = false;
    byte_list[i].payload_size = -1; //UNUSED PARAM BECAUSE THIS IS PAYLOAD  
  }
  //FIRST_AVAILABLE PRINT AND UPDATE
  int block_start = first_available;
  printf("%d\n",block_start+1); //necessary for output
  //add start header index, then payload start, then payload, then footer
  first_available = first_available + 1 + user_size + 1;
  
  HB new_footer;
  new_footer.data = new_header.data;
  new_footer.isAllocated = true;
  new_footer.isFooter = true;
  new_footer.isHeader = false;
  new_footer.payload_size = -1;
  byte_list[block_start + user_size+1] = new_footer;

  //about to go out of bounds OR next byte is already a header
  if (block_start + user_size >= 126 || byte_list[block_start + user_size+2].isHeader == true) {
    return; //stop creation of next header
  }
  
  // SET NEXT FREE HEADER IF NEXT BYTE NOT HEADER
  int footer_index = block_start + user_size + 1;
  int z = footer_index+2; //start at payload
  HB next_header;
  next_header.isHeader = true;
  next_header.isFooter = false;
  next_header.isAllocated = false;
  int sizeof_next_free = 0;
  while (z <= 127) {
    if (byte_list[z].isFooter) {
      break;
    }
    ++sizeof_next_free;
    ++z;
  }
  next_header.payload_size = sizeof_next_free;
  next_header.data = createHeader(sizeof_next_free, next_header.isAllocated);
  byte_list[footer_index+1] = next_header;
  //set next free footer
  int next_free_footer_index = footer_index+1 + 1+sizeof_next_free;
  // printf("DEBUG: NEXT FREE FOOTER INDEX IS: %d\n", next_free_footer_index);
  byte_list[next_free_footer_index].data = createHeader(sizeof_next_free, next_header.isAllocated);
  byte_list[next_free_footer_index].payload_size = sizeof_next_free;
  
}



void bestfit_malloc(HB * byte_list, char * user_arg_size) {
  //true for allocated
  int user_size = atoi(user_arg_size);
  /* FIND SMALLEST FREE BLOCK */
  int smallest_size_block = 500; 
  int smallest_payload_index = -1;
  
  for (int b = 0; b < 127; b++) {
    if((byte_list[b].isHeader == true) && (byte_list[b].isAllocated == false) && (byte_list[b].payload_size < smallest_size_block) && (user_size <= byte_list[b].payload_size)) {
    
      
      smallest_size_block = byte_list[b].payload_size; // set smallest_size_block to current smallest size
      smallest_payload_index = b; // CHANGE COULD BE b + 1
    }
  }
  // subtract the size of the free block to be filled by the size of the block the user is attempting to malloc
  int gap_size = (1+byte_list[smallest_payload_index].payload_size+1) - (1+user_size+1);

  // if the gap size is 1 or 2, add it back to user_size to utilize the space
  if (gap_size < 3) {
    user_size = user_size + gap_size;
  }


  //MAKE HEADER BYTE
  HB new_header;
  new_header.isAllocated = true;
  new_header.data = createHeader(user_size, new_header.isAllocated);
  new_header.isFooter = false;
  new_header.isHeader = true;
  new_header.payload_size = user_size;
  byte_list[smallest_payload_index] = new_header;
  if (smallest_payload_index >= 126) {
    return;
  }
  //PAYLOAD SPOTS SET TO 0
  //eg 0+1 = 1, i < 0 + 10 + 1 == 1-10 (inc)
  for (int i = smallest_payload_index+1; i < smallest_payload_index + user_size+1; ++i) {
    byte_list[i].data = 0;
    byte_list[i].isAllocated = true;
    byte_list[i].isHeader = false;
    byte_list[i].isFooter = false;
    byte_list[i].payload_size = -1; //UNUSED PARAM BECAUSE THIS IS PAYLOAD  
  }
  
  int block_start = smallest_payload_index;
  printf("%d\n",block_start+1); //necessary for output
  //add start header index, then payload start, then payload, then footer
  
  HB new_footer;
  new_footer.data = new_header.data;
  new_footer.isAllocated = true;
  new_footer.isFooter = true;
  new_footer.isHeader = false;
  new_footer.payload_size = -1; //alloc block, so -1
  byte_list[block_start + user_size+1] = new_footer;

  //about to go out of bounds OR next byte is already a header
  if (smallest_payload_index + user_size >= 126 || byte_list[block_start + user_size+2].isHeader == true) {
    return; //stop infinite loop
  }
  
  // SET NEXT FREE HEADER IF NEXT BYTE NOT HEADER
  int footer_index = block_start + user_size + 1;
  int z = footer_index+2; //start at payload
  HB next_header;
  next_header.isHeader = true;
  next_header.isFooter = false;
  next_header.isAllocated = false;
  int sizeof_next_free = 0;
  while (z <= 127) {
    if (byte_list[z].isFooter || byte_list[z].isHeader) {
      break;
    }
    ++sizeof_next_free;
    ++z;
  }
  next_header.payload_size = sizeof_next_free;
  next_header.data = createHeader(sizeof_next_free, next_header.isAllocated);
  byte_list[footer_index+1] = next_header;
  //set next free footer
  int next_free_footer_index = footer_index+1 + 1+sizeof_next_free;
  // printf("DEBUG: NEXT FREE FOOTER INDEX IS: %d\n", next_free_footer_index);
  byte_list[next_free_footer_index].data = createHeader(sizeof_next_free, next_header.isAllocated);
  byte_list[next_free_footer_index].payload_size = sizeof_next_free;
  
}

   