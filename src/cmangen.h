#include <stdio.h>

#define CMANGEN_VERSION "0.0.0"

#define DEF_OUT_MACHINE 0
#define DEF_OUT_HUMAN 1

typedef struct {
	char* header;
	int funcount;
	char** functions;
} HeaderDefs;

typedef struct {
	int valCount;
	char** keys;
	char** values;
} Meta;

typedef struct {
	int gzip;
	char* depDir;
	char* outDir;
	int fileCount;
	FILE** files;
	Meta* meta;
} Generator;

HeaderDefs* getDefinitions(char* path);

void printDefinition(int format, char* path);

int handleGen(int argc, char** argv);

void generate(Generator generator);
