#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_SIZE 32768

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
bool stringInSymbolTable(struct SymbolTable *dict, char *string); 


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

/*
 *  Applies LZW compression on the data of a given file, 
 *  outputting the result to a file named by the output parameter
 */
void encodeData(char *source, long sourceSize, char *output, struct SymbolTable *dict) {
    char cur[2] = ""; 
    char *prevSymbol = NULL;
    char prevChar[2]; 
    snprintf(prevChar, 2, "%s", cur);

    printf("%s", source);

    for (long i = 0 ; i < sourceSize ; i++) {
        snprintf(cur, 2, "%c", source[i]);

        printf("cur: %s\n", cur);
        printf("prev: %s\n", prevChar);

        snprintf(prevChar, 2, "%s", cur);

        if (stringInSymbolTable(dict, cur)) {
            printf("hello\n");
        } else {

        }
        printf("\n");
    }
}

/*
 *  Abstracted function - Checks if a given string is in the dictionary / table
 */
bool stringInSymbolTable(struct SymbolTable *dict, char *string) {
    for (int i = 0 ; i < MAX_SIZE ; i++) {
        if (dict->table[i] != NULL && strcmp(string, dict->table[i]) == 0) {
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
