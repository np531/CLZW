#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#define MAX_SIZE 32768
extern char* strdup(const char*);

struct KeyValue {
	char *key;
	int value;
	struct KeyValue *next;
};

struct SymbolTable {
	int head;
	char **table;
	struct KeyValue *hashMap[MAX_SIZE];
};

FILE *initData(int argc, char **argv); 
void initSourceString(FILE *f, long *sourceSize, char **sourceString); 
void encodeData(char *source, long sourceSize, char *output, struct SymbolTable *dict); 
void outputCode(long prevSize, char *prev, uint16_t code, FILE* f);
void writeToFile(char *output, bool byte, uint16_t code, char character); 
struct SymbolTable *initSymbolTable();
void freeSymbolTable(struct SymbolTable *dict); 
bool stringInSymbolTable(struct SymbolTable *dict, char *string, long pcSize, uint16_t* code); 

// HashMap Helper Functions
uint32_t hash(char *key) {
	// Hash algorithm inspired by djb2 by Dan Bernstein
	// Found to be quite efficient in reducing the number of collisions
	uint32_t hashValue = 5381;
	uint32_t cur;
	
	while ((cur = *key++)) {
		hashValue = cur + (hashValue + (hashValue << 5));
	}

	return hashValue % MAX_SIZE;
}

struct KeyValue *initKeyValue(char *key, int value) {
	struct KeyValue *pair = (struct KeyValue *)malloc(sizeof(struct KeyValue));
	if (pair == NULL) {
		printf("Unable to allocate memory for key-value pair\n");
		exit(1);
	}

	pair->next = NULL;
	pair->value =  value;
	pair->key = strdup(key);

	return pair;
}

/*
 *	Retrieves the value (array index) associated with the given key argument
 */
int get(struct SymbolTable *dict, char *key) {
	uint32_t index = hash(key);
	
	// Separate Chaining retrieval
	if (dict->hashMap[index] != NULL) {
		struct KeyValue *cur = dict->hashMap[index];
		while (cur != NULL) {
			if (strcmp(cur->key, key) == 0) {
				return cur->value;
			}
			cur = cur->next;
		}
	}
	return -1;
}

/*
 *	Returns whether the given key is in the symboltable
 */
bool keyInTable(struct SymbolTable *dict, char *key) {
	return get(dict, key) != -1;
}

/*
 *	Inserts the given key into the symboltable
 */
void insert(struct SymbolTable *dict, char *key) {
	if (dict->head < MAX_SIZE && !keyInTable(dict, key)) { 
		// Hash the key
		uint32_t index = hash(key);

		// Insert into hashmap (separate chaining)
		if (dict->hashMap[index] == NULL) {
			dict->hashMap[index] = initKeyValue(key, dict->head);
		} else {
			struct KeyValue *cur = dict->hashMap[index];
			while (cur->next != NULL) {
				cur = cur->next;
			}
			cur->next = initKeyValue(key, dict->head); 
		}
		
		// Insert into table
		dict->table[dict->head] = strdup(key);
		dict->head++;
	}

}

// Encode Algorithm
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
        printf("Incorrect arguments supplied\n");
        exit(1);
    }

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        printf("Given file does not exist\n");
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
        printf("Memory alloc for symbol table failed\n");
        exit(1);
    }

    newDict->head = 0;

    newDict->table = (char **)malloc(sizeof(char *) * MAX_SIZE);
    for (int i=0 ; i < MAX_SIZE ; i++) {
        newDict->table[i] = NULL;
		newDict->hashMap[i] = NULL;
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
void printNEntries(struct SymbolTable *d, int n) {
	for (int i = 0 ; i < n ; i++) {
		if (d->table[i] == NULL) {
			printf("    %d - NULL\n", i);
		} else {
			printf("    %d - %s\n", i, d->table[i]);
		}
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

    // String holding the concatenation of prev and cur (and size)
    char *pc = strdup("");
    long pcSize = 0;
    
    // code temporarily stores the binary representation of a dictionary reference 
    uint16_t code = 0;
    char *encodedString = (char*)malloc(sourceSize); 
    FILE *f = fopen(output, "wb");

	// Flag set to true if the current pc is in the dict (used to handle cases
	// when the last sequence of characters are in the dict, and thus nothing is output)
	bool inTable;

    for (long i = 0 ; i < sourceSize ; i++) {
        cur = source[i];
		inTable = false;

        // TODO - replace strlen with DS to store prev size (to handle null bytes)
        pcSize = strlen(prev) + sizeof(char) + 1;
        pc = (char*)realloc(pc, pcSize);
        snprintf(pc, pcSize, "%s%c", prev, cur);

        if (stringInSymbolTable(dict, pc, pcSize, &code)) {
			inTable = true;
            free(prev);
            prev = strdup(pc);
            prevSize = pcSize;
        } else {
            // Add pc to dict
			insert(dict, pc);

            // Output code(p)
			outputCode(prevSize, prev, code, f);

            // p = c
            prev = (char*)realloc(prev, sizeof(char)+1);
            snprintf(prev, 2, "%c", cur);
            prevSize = 2;

        }
    }

	if (inTable) {
		outputCode(prevSize, prev, code, f);
	} else {
		fwrite(&cur, 1,1,f);
	}

    free(pc);
    free(prev);
    fclose(f);
}

/*
 *	Given the required data, outputs either the raw prev string, or a 
 *	reference to the dict
 */
void outputCode(long prevSize, char *prev, uint16_t code, FILE* f) {
	if (prevSize <= 2) {
		fwrite(prev, 1, 1, f);
	} else if (prevSize == 3) {
		fwrite(prev, 1 , 2, f);	
	} else {
		code = (((1) << (sizeof(uint16_t)*8-1) ) | code);
		char LHS = (char)(code >> 8);
		char RHS = (char)code;
		fwrite(&LHS, sizeof(uint8_t), 1, f);
		fwrite(&RHS, sizeof(uint8_t), 1, f);
	}
}

/*
 *  Abstracted function - Checks if a given string is in the dictionary / table
 */
bool stringInSymbolTable(struct SymbolTable *dict, char *string, long pcSize, uint16_t* code) {
    // Case where the concat of prev and cur is a single character
    if (pcSize == 2) {
        return true;
    }

	// Hashmap implementation
	if (keyInTable(dict, string)) {
		*code = get(dict, string);
		return true;
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

