#include "parser.h"
#include "ast.h"
#include "cbackend.h"
#include "io.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	if(argc <= 1)
	{
		printf("No input files.\n");
		return -1;
	}
	else
	{
		FILE* input = fopen(argv[1], "r");
		char* data = read_file(input);
		fclose(input);
		ast_node *root = parse(data);
		compile(root, stdout);
		return 0;
	}
}