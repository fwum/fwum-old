#ifndef OPERATOR_H_
#define OPERATOR_H_
#include "semantic_analyzer.h"
#include "linked_list.h"
#include "util.h"

DEFSTRUCT(operator_node);

struct operator_node {
	char *data;
	statement_type operatorType;
};

linked_list *get_node();
bool is_operator(slice op);
#endif
