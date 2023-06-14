#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>

struct SymbolNode {
	char *symbol;
};

struct SymbolTable {
	int head;
	struct SymbolNode *table[32768];
};

FILE *initData(int argc, char **argv); 
char *initSourceString(FILE *f, long *sourceSize); 
void encodeData(char *source, long sourceSize, char *output, struct SymbolTable *dict); 
void writeToFile(char *output, bool byte, uint16_t code, char character); 
struct SymbolTable *initSymbolTable();
void freeSymbolTable(struct SymbolTable *dict); 


int main(int argc, char** argv) {
    FILE *f = initData(argc, argv);
    char *output = argv[2];
    long sourceSize = 0;

    char *source = initSourceString(f, &sourceSize);
    printf("sourceSize: %ld", sourceSize);
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
char *initSourceString(FILE *f, long *sourceSize) {
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    printf("fileSize: %ld", fileSize);
    char *sourceString = (char *)malloc(sizeof(fileSize + 1) * sizeof(char));

    fseek(f, 0, SEEK_SET);
    *sourceSize = fread(sourceString, sizeof(char), fileSize, f);
    sourceString[*sourceSize] = '\0';

    fclose(f);
    return sourceString;
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

    return newDict;
}

void freeSymbolTable(struct SymbolTable *dict) {
    for (int i = 0 ; i < 32768 ; i++) {
        if (dict->table[i] != NULL) {
            free(dict->table[i]->symbol);
        }
        free(dict->table[i]);
    }
    free(dict);
}

struct SymbolNode *newDictEntry(const char *string) {
    struct SymbolNode *newEntry = (struct SymbolNode *)malloc(sizeof(struct SymbolNode));
    if (newEntry == NULL) {
        printf("Unable to allocate SymbolNode");
        return NULL;
    }

    newEntry->symbol = (char *)malloc((strlen(string) + 1) * sizeof(char));
    if (newEntry->symbol == NULL) {
        printf("Malloc failed for dict entry symbol");
        return NULL;
    }
    strcpy(newEntry->symbol, string);

    return newEntry;
}

/*
 *  Applies LZW compression on the data of a given file, 
 *  outputting the result to a file named by the output parameter
 */
void encodeData(char *source, long sourceSize, char *output, struct SymbolTable *dict) {
    char cur; 
    char *prevSymbol = NULL;
    char prevChar = cur;
    for (long i = 0 ; i < sourceSize ; i++) {
        cur = source[i];
        printf("cur: %c\n", cur);
        printf("prev: %c\n", prevChar);
        prevChar = cur;
        // TODO - Start implementing trie data structure (or research better ds)
    }
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
