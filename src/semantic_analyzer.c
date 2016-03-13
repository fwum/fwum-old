#include "semantic_analyzer.h"
#include "slice.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

static void semantic_error(char *error, char *file, int line);

file_contents analyze(token_list *tokens)
{
	file_contents contents = {NULL, NULL};
	parse_token *current = tokens->head;
	while(current->next != NULL)
	{
		if(equals(current->data, new_slice("struct")))
		{
			tokens->head = current->next;
			struct_declaration *dec = analyze_struct(tokens);
			if(contents.head == NULL)
				contents.head = dec;
			else if(contents.tail == NULL)
			{
				contents.head->next = dec;
				contents.tail = dec;
			}
			else
			{
				contents.tail->next = dec;
				contents.tail = dec;
			}
		}
		current = current->next;
	}
	return contents;
}

struct_declaration *analyze_struct(token_list *tokens)
{
	parse_token *current = tokens->head;
	struct_declaration *dec = malloc(sizeof(*dec));
	dec->name = current->data;
	dec->head = dec->tail = NULL;
	dec->next = NULL;
	current = current->next;
	if(current->data.data[0] != '{')
		semantic_error("Expected opening brace after name in struct declaration.",
			current->origin.filename, current->origin.line);
	current = current->next;
	while(current->data.data[0] != '}')
	{
		struct_member *member = malloc(sizeof(*member));
		if(current->type != WORD)
			semantic_error("Struct members must be declared as <type> <value>;",
				current->origin.filename, current->origin.line);
		member->type = current->data;
		current = current->next;
		if(current->type != WORD)
			semantic_error("Struct members must be declared as <type> <value>;",
				current->origin.filename, current->origin.line);
		member->name = current->data;
		current = current->next;
		if(current->data.data[0] != ';')
			semantic_error("Struct members must be declared as <type> <value>;",
				current->origin.filename, current->origin.line);
		member->next = NULL;
		if(dec->head == NULL)
			dec->head = member;
		else if(dec->tail == NULL)
			dec->head->next = dec->tail = member;
		else
			dec->tail->next = member;
		current = current->next;
	}
	tokens->head = current->next;
	dec->next = NULL;
	return dec;
}

void dump(file_contents contents)
{
	struct_declaration *current = contents.head;
	while(current != NULL)
	{
		printf("STRUCT: %s\n", evaluate(current->name));
		struct_member *member = current->head;
		while(member != NULL)
		{
			printf("MEMBER: NAME: %s | TYPE: %s\n", evaluate(member->type), evaluate(member->name));
			member = member->next;
		}
		current = current->next;
	}
}

static void semantic_error(char *error, char *file, int line)
{
	fprintf(stderr, "Error encountered while analyzing %s at line %d:\n%s\n", file, line, error);
	exit(-1);
}