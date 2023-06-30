# CLZW
LZW Compression and Decompression programs written in C
Uses a hybrid hashmap + array data structure to perform very quick LZW dictionary lookups

# Compilation (tested on debian linux)
```
gcc lencode.c -o lencode
gcc ldecode.c -o ldecode
```

# Usage
Compress the given file:
```
./lencode <source file> <output file>
```

Decompress the given file:
```
./ldecode <compressed file> <output file>
```
