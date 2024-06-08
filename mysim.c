/*************************
Date: 11/4/2023
Class: CS4541
Assignment: Cache Lab
Author(s): Ramses Larabel
**************************/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

struct Data {
    long op_address;
    int size;
    char op_field;
};

struct cacheLine {
	uint64_t tag;
	int valid;
};

struct Data* parseFile(const char*, int*);
int cacheSimulator(struct cacheLine*, uint64_t, uint64_t, int, int);
void printVerbose(char, uint64_t, int, int, int);
void hitCounter(int, int*, int*, int*);

int main(int argc, char* argv[]) {
	int var_count = 0;
	int s, E, b, S, i, j;
	char t[50];
	bool h = false, v = false, arg_error = false;
	for  (i = 1; i < argc; i++) {
		if (i == 1){
			if (strcmp(argv[1], "-hv") == 0) {
				h = true;
				v = true;
			}
			else if (strcmp(argv[1], "-h") == 0) h = true;
			else if (strcmp(argv[1], "-v") == 0) v = true;
			else if (strcmp(argv[1], "-s") == 0) {
				s = atoi(argv[2]);
				var_count++;
			}
			else arg_error = true;
		}
		else {
			if (strcmp(argv[i], "-s") == 0) {
				var_count++;
				s = atoi(argv[i+1]);
			}
			else if (strcmp(argv[i], "-E") == 0) {
				var_count++;
				E = atoi(argv[i+1]);
			}
			else if (strcmp(argv[i], "-b") == 0) {
				var_count++;
				b = atoi(argv[i+1]);
			}
			else if (strcmp(argv[i], "-t") == 0) {
				var_count++;
				strcpy(t, argv[i+1]);
			}
		}
	}
	if (arg_error || (var_count != 4) || h) {
		if (h == false) printf("Bad file name\n");
		printf("Usage: ./mysim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
		printf("-h: Optional help flag that prints usage info\n");
		printf("-v: Optional verbose flag that displays trace info\n");
		printf("-s <s>: Number of set index bits (S = 2 s is the number of sets)\n");
		printf("-E <E>: Associativity (number of lines per set)\n");
		printf("-b <b>: Number of block bits (B = 2 b is the block size)\n");
		printf("-t <tracefile>: Name of the valgrind trace to replay\n");
	}
	else {
		char filename[100] = "traces/";
		strcat(filename, t);
		int n, loadResult, storeResult, hitCount = 0, missCount = 0, evictionCount = 0;
		uint64_t sData = 0, tData = 0, mask = 0;
		struct Data* data = parseFile(filename, &n);
		S = 2 << (s - 1);

		// Initializing the cache and setting valid bit to 0
		struct cacheLine *cache = malloc(sizeof(struct cacheLine) * S * E);
		for (i = 0; i < E; i++) {
			for(j = 0; j < S; j++) {
				cache[(i * S) + j].valid = 0;
			}
		}

		// Going through each line in the trace file
		for (i = 0; i < n; i++) {
			// Finding s, given by the address, to know where to look in the cache
			mask = (1 << s) - 1;
			sData = (int) data[i].op_address;
			sData = sData >> b;
			sData = sData & mask;
			// Finding t, given by the address, to know what to look for in the cache
			tData = (int) data[i].op_address;
			tData = tData >> (s + b);
        		if(data[i].op_field == 'L') {
				loadResult = cacheSimulator(cache, sData, tData, S, E);
				if (v == true) printVerbose(data[i].op_field, data[i].op_address, data[i].size, loadResult, -1);
				if (loadResult == 0) hitCount++;
        			else if (loadResult == 1) missCount++;
        			else {
					evictionCount++;
					missCount++;
				}
			}
			else if(data[i].op_field == 'S') {
				storeResult = cacheSimulator(cache, sData, tData, S, E);
				if (v == true) printVerbose(data[i].op_field, data[i].op_address, data[i].size, -1, storeResult);
				if (storeResult == 0) hitCount++;
        			else if (storeResult == 1) missCount++;
        			else {
					evictionCount++;
					missCount++;
				}
			}
			else if(data[i].op_field == 'M') {
				loadResult = cacheSimulator(cache, sData, tData, S, E);
				storeResult = cacheSimulator(cache, sData, tData, S, E);
				if (v == true) printVerbose(data[i].op_field, data[i].op_address, data[i].size, loadResult, storeResult);
				if (loadResult == 0) hitCount++;
        			else if (loadResult == 1) missCount++;
        			else {
					evictionCount++;
					missCount++;
				}
				if (storeResult == 0) hitCount++;
        			else if (storeResult == 1) missCount++;
        			else {
					evictionCount++;
					missCount++;
				}
			}
		}
		printf("hits:%d misses:%d evictions:%d\n", hitCount, missCount, evictionCount);
		free(data);
        	free(cache);
	}
	return 0;
}

struct Data* parseFile(const char* filename, int* numRecords) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Unable to open the file: %s\n", filename);
        exit(1);
    }

    // Define an initial array size 
    int capacity = 10;
    struct Data* data = (struct Data*)malloc(capacity * sizeof(struct Data));
    if (data == NULL) {
        fprintf(stderr, "Memory allocation error.\n");
        exit(1);
    }

    int count = 0;
    char line[256]; // Assuming a maximum line length of 255 characters

    while (fgets(line, sizeof(line), file)) {
        // Parse the line based on the format
        long op_address;
        int size;
        char op_field;

        if (sscanf(line, " %c %lx,%d\n", &op_field, &op_address, &size) == 3 ||
            sscanf(line, "I %lx,%d\n", &op_address, &size) == 2) {
            // Check if the array needs to be resized
            if (count >= capacity) {
                capacity *= 2;
                data = (struct Data*)realloc(data, capacity * sizeof(struct Data));
                if (data == NULL) {
                    fprintf(stderr, "Memory allocation error.\n");
                    exit(1);
                }
            }

            // Store the parsed data in the struct array
            data[count].op_field = op_field;
            data[count].op_address = op_address;
            data[count].size = size;

            count++;
        } else {
            fprintf(stderr, "Invalid line in the file: %s", line);
        }
    }

    fclose(file);

    // Update the number of records parsed and return the array
    *numRecords = count;
    return data;
}

// Input: Simulated Cache, set index value given by the address, tag value given by address, total set, total lines per set
// Output: an interger value: 0 a hit, 1 a miss, 2 a miss eviction
int cacheSimulator(struct cacheLine* cache, uint64_t sData, uint64_t tData, int S, int E) {
	int j, result = 2;
	// Iterates through each line in the given set to determine hit or miss
	// If line is not valid put data there because the rest of the lines also won't be valid
	for (j = 0; j < E; j++) {
		if (cache[sData + S * j].valid == 1) {
			if (cache[sData + S * j].tag == tData) {
				result = 0;
			}
		} else {
			cache[sData + S * j].tag = tData;
			cache[sData + S * j].valid = 1;
			result = 1;
		}
	}
	// Handling eviction misses: This means address did not hit and there was no room
	// Popping the first line of the set and apending the the new tag
	if (result == 2) {
		for(j = 1; j < E; j++) {
			cache[sData + S * (j - 1)].tag = cache[sData + S * j].tag;
		}
		cache[sData + S * (E - 1)].tag = tData;
	}
	return result;
}

void printVerbose(char op_field, uint64_t op_address, int op_size, int loadResult, int storeResult) {
	char result1[25], result2[25];
	if (loadResult == 0) strcpy(result1, "hit");
	else if (loadResult == 1) strcpy(result1, "miss");
	else if (loadResult == 2) strcpy(result1, "eviction miss");
	if (storeResult == 0) strcpy(result2, "hit");
        else if (storeResult == 1) strcpy(result2, "miss");
        else if (storeResult == 2) strcpy(result2, "eviction miss");

	if (op_field == 'L') printf("L %lx,%d %s\n", op_address, op_size, result1);
	else if (op_field == 'S') printf("S %lx,%d %s\n", op_address, op_size, result2);
	else if (op_field == 'M') printf("M %lx,%d %s %s\n", op_address, op_size, result1, result2);
	printf("\n");
}
