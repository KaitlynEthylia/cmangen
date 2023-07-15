#include "cmangen.h"
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* strToHeap(char* string) {
	int len = strlen(string);
	char* hstring = malloc(len + 1);
	strcpy(hstring, string);
	return hstring;
}

char** argsFromFile(char* path, int* pargc) {
	if (path == NULL)
		return NULL;

	FILE* conf = fopen(path, "r");
	fseek(conf, 0, SEEK_END);
	long len = ftell(conf);
	rewind(conf);

	char content[len + 1];
	content[len] = '\0';

	fread(content, sizeof(char), len, conf);
	fclose(conf);

	char contentcpy[len + 1];
	strcpy(contentcpy, content);

	char* svptr = NULL;
	int argc = 2;
	strtok_r(contentcpy, " \t\n\r", &svptr);
	do
		argc++;
	while (strtok_r(NULL, " \t\n\r", &svptr) != NULL);

	svptr = NULL;
	char** argv = calloc(argc, sizeof(char*));
	argv[0] = "cmangen";
	argv[1] = "-G";
	argv[2] = strToHeap(strtok_r(content, " \t\n\r", &svptr));
	for (int i = 3; i < argc; i++)
		argv[i] = strToHeap(strtok_r(NULL, " \t\n\r", &svptr));

	*pargc = argc;
	return argv;
}

void printHelp(int status) {
	printf("USAGE...\n");
	exit(status);
}

void handleDefs(int argc, char** argv) {
	int outFormat = isatty(1);

	struct option* defOpts = calloc(2, sizeof(struct option));
	defOpts[0].name = "format";
	defOpts[0].val = 'f';
	defOpts[0].has_arg = required_argument;
	int c;
	while ((c = getopt_long(argc, argv, "f:", defOpts, NULL)) != -1) {
		switch (c) {
		case 'f':
			if (!strcmp(optarg, "human"))
				outFormat = DEF_OUT_MACHINE;
			else if (!strcmp(optarg, "machine"))
				outFormat = DEF_OUT_HUMAN;
			break;
		default:
			printHelp(EXIT_FAILURE);
		}
	}
	free(defOpts);

	if (optind >= argc)
		printHelp(EXIT_FAILURE);
	for (; optind < argc; optind++)
		printDefinition(outFormat, argv[optind]);
	printf("\n");
}

int parseMetaArgs(int argc, char** argv, char*** keys, char*** values) {
	int count = 0;
	for (int i = optind; i < argc; i++)
		count += strlen(argv[i]) + 1;

	char buffer[count + 1];
	buffer[0] = '\0';
	for (; optind < argc; optind++) {
		strcat(buffer, argv[optind]);
		strcat(buffer, " ");
	}

	char buffercpy[count + 1];
	strcpy(buffercpy, buffer);

	char* svptr = NULL;
	int tokenc = 0;
	strtok_r(buffercpy, " =", &svptr);
	do
		tokenc++;
	while (strtok_r(NULL, " =", &svptr));

	if (tokenc % 2 != 0)
		printHelp(EXIT_FAILURE);
	tokenc /= 2;

	*keys = calloc(tokenc, sizeof(char*));
	*values = calloc(tokenc, sizeof(char*));

	for (int i = 0; i < tokenc; i++) {
		char* argkey;
		if (i == 0)
			argkey = strToHeap(strtok_r(buffer, " =", &svptr));
		else
			argkey = strToHeap(strtok_r(NULL, " =", &svptr));
		*values[i] = strToHeap(strtok_r(NULL, " =", &svptr));

		if (strncmp(argkey, "-M", 2)) {
			char* key = argkey + 2;
			int len = strlen(key) + 1;
			memmove(argkey, key, len);
			*keys[i] = realloc(argkey, len);
			continue;
		}

		if (strncmp(argkey, "--meta-", 7)) {
			char* key = argkey + 7;
			int len = strlen(key) + 1;
			memmove(argkey, key, len);
			*keys[i] = realloc(argkey, len);
			continue;
		}

		printf("Failed to parse argument '%s', Meta values should come after "
		       "all other arguments.",
		       argkey);
		printHelp(EXIT_FAILURE);
	}

	return tokenc;
}

FILE* fileFromDirent(char* dirent, char* dir, char* mode) {
	char path[strlen(dirent) + strlen(dir) + 2];
	sprintf(path, "%s/%s", dir, dirent);
	return fopen(path, mode);
}

int main(int argc, char** argv) {
	optind = 0;
	if (argc < 2) {
		argc = 3;
		char* argv[] = {"cmangen", "-Gc", "Mangen"};
		return main(argc, argv);
	}

	struct option* options = calloc(5, sizeof(struct option));
	options[0].name = "help";
	options[0].val = 'h';
	options[1].name = "version";
	options[1].val = 'V';
	options[2].name = "generate";
	options[2].val = 'G';
	options[3].name = "definitions";
	options[3].val = 'D';

	char opt = getopt_long(argc, argv, "hVGD", options, NULL);
	free(options);

	switch (opt) {
	default:
		printHelp(EXIT_FAILURE);
		break;
	case 'h':
		printHelp(EXIT_SUCCESS);
		break;
	case 'V':
		printf("Version: cmangen %s\n", CMANGEN_VERSION);
		break;
	case 'D':
		handleDefs(argc, argv);
		break;
	case 'G':
		return handleGen(argc, argv);
	}
	return 0;
}

int handleGen(int argc, char** argv) {
	char* depDir = NULL;
	char* outDir = ".";
	int gzip = 0;
	char* readDir = NULL;

	struct option* genOpts = calloc(6, sizeof(struct option));
	genOpts[0].name = "gzip";
	genOpts[0].val = 'g';
	genOpts[1].name = "directory";
	genOpts[1].val = 'd';
	genOpts[1].has_arg = required_argument;
	genOpts[2].name = "config";
	genOpts[2].val = 'c';
	genOpts[2].has_arg = required_argument;
	genOpts[3].name = "output";
	genOpts[3].val = 'o';
	genOpts[3].has_arg = required_argument;
	genOpts[4].name = "makedeps";
	genOpts[4].val = 'm';
	genOpts[4].has_arg = required_argument;
	int c;
	while ((c = getopt_long(argc, argv, "gd:c:o:m:VM", genOpts, NULL)) != -1) {
		switch (c) {
		case 'g':
			gzip = 1;
			break;
		case 'm':
			depDir = optarg;
			break;
		case 'o':
			outDir = optarg;
			break;
		case 'd':
			readDir = optarg;
			break;
		case 'c': {
			int argc2;
			char** argv2 = argsFromFile(optarg, &argc2);
			int difference = argc - optind + 1;
			argv2 = realloc(argv2, (argc2 + difference) * sizeof(char));
			for (int i = 0; i < difference; i++)
				argv2[argc2 + i] = argv[optind + i];
			argc2 += difference - 1;

			int ret = main(argc2, argv2);
			for (int i = 0; i < argc2; i++)
				free(argv2[i]);
			free(argv2);
			return ret;
		}
		case 'V':
		case 'M':
			break;
		default:
			printHelp(1);
		}

		if (c == 'V' || c == 'M') {
			optind -= 1;
			break;
		}
	}
	free(genOpts);

	char** keys;
	char** values;
	int margc = argc;
	// Hide the last argument if a file name is expected
	if (readDir == NULL)
		margc--;
	int valCount = parseMetaArgs(margc, argv, &keys, &values);

	Meta* meta = malloc(sizeof(Meta));
	meta->keys = keys;
	meta->values = values;
	meta->valCount = valCount;

	int fileCount = 0;
	FILE** files = NULL;
	if (readDir == NULL) {
		fileCount = 1;
		files = malloc(sizeof(FILE*));
		files[0] = fopen(argv[argc - 1], "r");
	} else {
		DIR* dir = opendir(readDir);
		struct dirent* f;
		while ((f = readdir(dir))) {
			int namelen = strlen(f->d_name);
			if (f->d_name[namelen - 1] == 'c' &&
			    f->d_name[namelen - 2] == '.' && f->d_type == DT_REG) {
				files = realloc(files, ++fileCount);
				files[fileCount - 1] = fileFromDirent(f->d_name, readDir, "r");
			}
		}
	}

	Generator genArgs = {.gzip = gzip,
	                     .depDir = depDir,
	                     .outDir = outDir,
	                     .fileCount = fileCount,
	                     .files = files,
	                     .meta = meta};

	generate(genArgs);

	return 0;
}
