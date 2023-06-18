#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#define MAX_SIZE 32768
extern char* strdup(const char*);

struct SymbolTable {
	int head;
	char **table;
};

FILE *initData(int argc, char **argv); 
void initSourceString(FILE *f, long *sourceSize, char **sourceString); 
void encodeData(char *source, long sourceSize, char *output, struct SymbolTable *dict); 
void writeToFile(char *output, bool byte, uint16_t code, char character); 
struct SymbolTable *initSymbolTable();
void freeSymbolTable(struct SymbolTable *dict); 
bool stringInSymbolTable(struct SymbolTable *dict, char *string, long pcSize, uint16_t* code); 


int main(int argc, char** argv) {
    FILE *f = initData(argc, argv);
    char *output = argv[2];
    long sourceSize = 0;
    char *source; 

    initSourceString(f, &sourceSize, &source);
    struct SymbolTable *dict = initSymbolTable();
    encodeData(source, sourceSize, output, dict);

    freeSymbolTable(dict);
    return 0;
}

/*
 * initData ensures enough arguments are given and that they are valid
 */
FILE *initData(int argc, char **argv) {
    if (argc != 3) {
        printf("Incorrect arguments supplied");
        exit(1);
    }

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        printf("Given file does not exist");
        exit(1);
    }

    return f;
}

/*
 *  Reads all the data in from the input file into a
 *  dynamically allocated string.
 */ 
void initSourceString(FILE *f, long *sourceSize, char **sourceString) {
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    *sourceString = (char *)malloc((fileSize + 1) * sizeof(char));

    fseek(f, 0, SEEK_SET);
    *sourceSize = fread(*sourceString, sizeof(char), fileSize, f);
    fclose(f);

    (*sourceString)[*sourceSize] = '\0';
}

/*
 * Allocates memory to the heap for the symbol table,
 * which is my implementation of the LZW heap.
 */
struct SymbolTable *initSymbolTable() {
    struct SymbolTable *newDict = (struct SymbolTable *)malloc(sizeof(struct SymbolTable));
    if (newDict == NULL) {
        printf("Memory alloc for symbol table failed");
        exit(1);
    }

    newDict->head = 0;

    newDict->table = (char **)malloc(sizeof(char *) * MAX_SIZE);
    for (int i=0 ; i < MAX_SIZE ; i++) {
        newDict->table[i] = NULL;
    }

    return newDict;
}

void freeSymbolTable(struct SymbolTable *dict) {
    for (int i = 0 ; i < 32768 ; i++) {
        if (dict->table[i] != NULL) {
            free(dict->table[i]);
        }
    }
    free(dict->table);
    free(dict);
}

void printUInt16AsBinary(uint16_t value) {
    for (int i = sizeof(uint16_t) * 8 - 1; i >= 0; i--) {
        uint16_t mask = (uint16_t)1 << i;
        int bit = (value & mask) ? 1 : 0;
        printf("%d", bit);
    }
    printf("\n");
}

void printCharAsBinary(char ch) {
    for (int i = 7; i >= 0; i--) {
        char bit = (ch >> i) & 1;
        printf("%d", bit);
    }
}

/*
 *  Applies LZW compression on the data of a given file, 
 *  outputting the result to a file named by the output parameter
 */
void encodeData(char *source, long sourceSize, char *output, struct SymbolTable *dict) {
    // String holding p (and size)
    char *prev = (char*)calloc(1, sizeof(char)+1);
    long prevSize = 0;
    char cur = '\0'; 

    // String holding the concatenation of p and c (and size)
    char *pc = strdup("");
    long pcSize = 0;
    
    // code temporarily stores the binary representation of a dictionary reference 
    uint16_t code = 0;
    char LHS;
    char RHS;
    char *encodedString = (char*)malloc(sourceSize); 
    FILE *f = fopen(output, "wb");

    for (long i = 0 ; i < sourceSize ; i++) {
        cur = source[i];

        printf("cur: %c\n", cur);
        printf("prev: %s\n", prev);


        // TODO - Handle case when dictionary is full
        // TODO - replace strlen with DS to store prev size (to handle null bytes)
        pcSize = strlen(prev) + sizeof(char) + 1;
        pc = (char*)realloc(pc, pcSize);
        snprintf(pc, pcSize, "%s%c", prev, cur);
        printf("%s\n", pc);
        printf("%ld\n", prevSize);

        if (stringInSymbolTable(dict, pc, pcSize, &code)) {
            free(prev);
            prev = strdup(pc);
            prevSize = pcSize;
        } else {
            // Add pc to dict
            dict->table[dict->head] = strdup(pc);
            dict->head++;

            // Output code(p)
            if (prevSize <= 2) {
                /* strlcat(encodedString, prev, sourceSize); */
                fwrite(prev, 1, 1, f);
            } else {
                code = (((1) << (sizeof(uint16_t)*8-1) ) | code);
                printUInt16AsBinary(code);
                LHS = (char)(code >> 8);
                RHS = (char)code;
                /* strncat(encodedString, &LHS, 1); */
                /* strncat(encodedString, &RHS, 1); */
                fwrite(&LHS, sizeof(uint8_t), 1, f);
                fwrite(&RHS, sizeof(uint8_t), 1, f);
            }

            // p = c
            prev = (char*)realloc(prev, sizeof(char)+1);
            snprintf(prev, 2, "%c", cur);
            prevSize = 2;

        }
        printf("\n");
    }
    free(pc);
    free(prev);
	printf("Num entries: %d", dict->head);
    printf("\nFINAL-%s\n", encodedString);
	for (int i=0;i<10;i++) {
		if (dict->table[i] != NULL) {
			printf("%d: %s\n",i,dict->table[i]);
		}
	}
    /* fwrite(encodedString, strlen(encodedString), 1, f); */
    fclose(f);
}

/*
 *  Abstracted function - Checks if a given string is in the dictionary / table
 */
bool stringInSymbolTable(struct SymbolTable *dict, char *string, long pcSize, uint16_t* code) {
    // Case where the concat of prev and cur is a single character
    if (pcSize == 2) {
        return true;
    }

    // Otherwise perform a linear search of the symbolTable table
    for (int i = 0 ; i < MAX_SIZE ; i++) {
        if (dict->table[i] != NULL && strcmp(string, dict->table[i]) == 0) {
            *code = i;
            return true;
        }
    }
    return false;
}

/*
 *  Given some data and the name of an output file, writes num_bytes of data
 *  to output.
 *  Example - If num_bytes = 2 then all of data is written
 *            If num_bytes = 1 then only a single byte is written
 */
void writeToFile(char *output, bool byte, uint16_t code, char character) {
    FILE *f = fopen(output, "ab");

    if (byte) {
        printf("%c", character);
        fprintf(f, "%c", character);
    } else {
        fprintf(f, "%c%c", code >> 8, code);
        printf("%c", (char)code);
    }

}
