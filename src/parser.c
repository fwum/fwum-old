#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "slice.h"
#include "ast.h"

ast_node* parse_toplevel(slice str);
ast_node* parse_function(slice data);
ast_node* parse_struct(slice data);
ast_node* parse(char* data);

int main()
{
	ast_node *root = parse("import x; using a; c getC(a : b) { if(a == b) { let value = 1 + call(param) + 3; } } struct c{a:b;} import f;");
	printf("%s\n", to_string(root));
	return 0;
}

ast_node* parse(char* data) {
	int len = strlen(data);
	slice buffer = make_slice(data, 0, 0);
	int brace_level = 0;
	ast_node *root = new_node(ROOT, "");
	for(int i = 0; i < len; i++)
	{
		buffer.end += 1;
		if(data[i] == '{')
			brace_level += 1;
		else if(data[i] == '}')
		{
			brace_level -= 1;
			if(brace_level == 0)
			{
				slice start = buffer;
				start.begin++;
				start.end = start.begin + 6; //length of "struct"
				if(equals_string(start, "struct"))
					add_child(root, parse_struct(buffer));
				else
					add_child(root, parse_function(buffer));
				buffer.begin = buffer.end;
			}
		}
		else if(data[i] == ';' && brace_level == 0)
		{
			add_child(root, parse_toplevel(buffer));
			buffer.begin = buffer.end;
		}
	}
	return root;
}

ast_node* parse_toplevel(slice data)
{
	ast_type type;
	int displace = 0;
	while(is_whitespace(get(data, 0)))
		data.begin++;
	if(starts_with(data, new_slice("import")))
	{
		type = IMPORT;
		displace = strlen("import ");
	} else if(starts_with(data, new_slice("using")))
	{
		type = USING;
		displace = strlen("using ");
	}
	data.begin += displace;
	data.end--; //Remove semicolon
	ast_node *toplevel = new_node(type, evaluate(data));
	return toplevel;
}

ast_node* parse_block(slice data);
ast_node* parse_vartype(slice data);
ast_node* parse_call(slice data);
ast_node* parse_val(slice data);

ast_node* parse_function(slice data)
{
	while(is_whitespace(get(data, 0)))
		data.begin++;
	slice type = clone_slice(data, data.begin, data.begin);
	while(!is_whitespace(get_last(type)))
		type.end++;
	slice name = clone_slice(data, type.end + 1, type.end + 1);
	while(is_whitespace(get(name, 0)))
		name.begin++;
	name.end = name.begin;
	while(is_identifier(get_last(name)))
		name.end++;
	ast_node *root = new_node(FUNC, evaluate(name));
	ast_node *returnType = new_node(TYPE, evaluate(type));
	add_child(root, returnType);
	slice vartype = name;
	vartype.begin = vartype.end = name.end + 1;
	while(get(vartype, -1) != '(')
		vartype.begin++;
	while(is_whitespace(get(vartype, 0)))
		vartype.begin++;
	vartype.end = vartype.begin + 1;
	while(get(vartype, 0) != ')' && get_last(vartype) != ')')
	{
		vartype.end++;
		char current = get_last(vartype);
		if(current == ';' || current == ')')
			add_child(root, parse_vartype(vartype));
	}
	slice block = clone_slice(vartype, vartype.end, vartype.end);
	while(get(block, -1) != '{')
		block.begin++;
	int block_level = 0;
	block.end = block.begin;
	while(!(block_level == 0 && get_last(block) == '}')) {
		char current = get_last(block);
		if(current == '{') block_level++;
		if(current == '}') block_level--;
		block.end++;
	}
	add_child(root, parse_block(block));
	return root;
}

ast_node* parse_controlflow(slice data);

ast_node* parse_block(slice data)
{
	slice current = clone_slice(data, data.begin, data.end);
	ast_node *root = new_node(BLOCK, "");
	int block_level = 0;
	for(int i = data.begin; i < data.end; i++)
	{
		current.end = i;
		if(data.data[i] == '{') block_level++;
		if(data.data[i] == '}')
		{
			block_level--;
			if(block_level == 0)
			{
				add_child(root, parse_controlflow(current));
				current.begin = current.end;
			}
		}
		if(block_level == 0 && data.data[i] == ';')
		{
			add_child(root, parse_val(current));
			current.begin = current.end;
		}
	}
	return root;
}

ast_node* parse_controlflow(slice data)
{
	while(is_whitespace(get(data, 0)))
		data.begin++;
	slice control_name = clone_slice(data, data.begin, data.begin);
	while(!is_whitespace(get(control_name, control_name.end - control_name.begin))
		&& get(control_name, control_name.end - control_name.begin) != '(')
		control_name.end++;
	ast_node *root = new_node(CONTROL, evaluate(control_name));
	if(equals(control_name, new_slice("if")) || equals(control_name, new_slice("while")))
	{
		slice header = clone_slice(data, control_name.end + 1, control_name.end + 1);
		while(get(header, header.end - header.begin) != ')')
			header.end++;
		add_child(root, parse_val(header));
		data.begin = header.end + 1;
		bool opened_bracket = false;
		while(is_whitespace(get(data, 0)) || (get(data, 0) == '{' && !opened_bracket))
		{
			if(get(data, 0) == '{')
				opened_bracket = true;
			data.begin++;
		}
	}
	add_child(root, parse_block(data));
	return root;
}

ast_node* parse_assignment(slice assign)
{
	slice data = assign;
	ast_type type;
	while(is_whitespace(get(data, 0)))
		data.begin++;
	ast_node *root;
	if(starts_with(data, new_slice("let")))
	{
		type = BIND;
		int i = 0;
		while(get(data, i) != '=')
		{
			if(get(data, i) == ':')
			{
				type = TYPED_BIND;
				break;
			}
			i++;
		}
		data.begin += 4; //length of let and at least 1 whitespace token
		while(is_whitespace(get(data, 0)))
			data.begin++;
		data.end = data.begin+1;
		while(!is_whitespace(get(data, data.end - data.begin)) && get(data, data.end - data.begin) != ':')
			data.end++;
		char *name = evaluate(data);
		root = new_node(type, name);
		if(type == TYPED_BIND)
		{
			data.begin = data.end;
			while(get(data, -1) != ':')
				data.begin++;
			data.end = data.begin + 1;
			while(get(data, data.end - data.begin + 1) != '=')
				data.end++;
			char *typeStr = evaluate(data);
			ast_node *typeNode = new_node(TYPE, typeStr);
			add_child(root, typeNode);
		}
		data.begin = data.end + 2;
		data.end = assign.end;
	}
	else
	{
		type = ASSIGN;
		data.end = data.begin;
		while(get(data, data.end - data.begin + 1) != '=')
			data.end++;
		char *name = evaluate(data);
		root = new_node(type, name);
		data.begin = data.end + 2;
		data.end = assign.end;
	}
	add_child(root, parse_val(data));
	return root;
}

slice operator_token(slice data);

ast_node* parse_val(slice data)
{
	while(is_whitespace(get(data, 0)))
		data.begin++;
	while(is_whitespace(get(data, data.end - data.begin - 1)))
		data.end--;
	if(is_number_literal(data))
		return new_node(NUMBER, evaluate(data));
	else if(is_string_literal(data))
		return new_node(STRING, evaluate(data));
	else if(is_identifier_literal(data))
		return new_node(VAR, evaluate(data));
	else
	{
		//Determine if the value is a function call
		bool startsID = false, precedingParen = false;
		for(int i = data.begin; i < data.end; i++)
		{
			if(is_identifier(data.data[i]))
				startsID = true;
			if(data.data[i] == '(' && startsID)
			{
				precedingParen = true;
				break;
			}
			if(!is_whitespace(data.data[i]) && !is_identifier(data.data[i]))
			{
				startsID = false;
				break;
			}
		}
		//Starts with a function call and ends without any other operation
		if(startsID && precedingParen && get(data, data.end - data.begin - 1) == ')')
		{
			slice call = clone_slice(data, data.begin, data.end);
			int paren_level = 0;
			call.end = call.begin;
			while(!(paren_level == 1 && get(call, call.end - call.begin) == ')'))
			{
				char current = get(call, call.end - call.begin);
				if(current == '(') paren_level++;
				if(current == ')') paren_level--;
				call.end++;
			}
			return parse_call(call);
		}
		else
		{
			//Determine if the current value is an assignment or not
			//TODO: INCLUDE COMPARISON OPERRATORS
			printf("%s\n", evaluate(operator_token(data)));
			slice token = operator_token(data);
			slice operator = clone_slice(token, token.end, token.end + 1);
			while(is_whitespace(get(operator, 0)))
				operator.begin++;
			operator.end = operator.begin;
			while(!is_whitespace(get(operator, operator.end - operator.begin))
				&& !is_identifier(get(operator, operator.end - operator.begin)))
				operator.end++;
			if(equals(operator, new_slice("=")))
				return parse_assignment(data);
			ast_node *root, *operand1, *operand2;
			root = new_node(OPERATOR, evaluate(operator));
			operand1 = parse_val(token);
			operand2 = parse_val(clone_slice(data, operator.end, data.end));
			add_child(root, operand1);
			add_child(root, operand2);
			return root;
		}
	}
}

slice operator_token(slice data)
{
	slice result = clone_slice(data, data.begin, data.begin);
	int paren_level = 0;
	while(result.end < data.end)
	{
		result.end++;
		char current = get(result, result.end - result.begin);
		if(current == '(') paren_level++;
		if(current == ')') paren_level--;
		if(paren_level == 0 && !is_whitespace(current) && !is_identifier(current)
			&& current != '(' && current != ')' && current != ',')
			break;
	}
	return result;
}

ast_node* parse_call(slice data)
{
	slice name = clone_slice(data, data.begin, data.end);
	while(is_whitespace(get(name, 0)))
		name.begin++;
	name.end = name.begin;
	while(!is_whitespace(get(name, name.end - name.begin)) && get(name, name.end - name.begin) != '(')
		name.end++;
	ast_node *root = new_node(CALL, evaluate(name));
	slice parameter = clone_slice(data, data.begin, data.end);
	while(get(parameter, -1) != '(')
		parameter.begin++;
	parameter.end = parameter.begin;
	for(int i = data.begin; i < data.end; i++)
	{
		parameter.end++;
		char current = get(parameter, parameter.end - parameter.begin);
		if(current == ',' || current == ')')
			add_child(root, parse_val(parameter));
	}
	return root;
}

ast_node* parse_struct(slice data)
{
	data.begin += 7;//length of struct and a space
	while(is_whitespace(get(data, 0)))
		data.begin++;
	slice structname = data;
	int i = 0;
	while(get(structname, i) != '{')
		i++;
	structname.end = structname.begin + i;
	ast_node *root = new_node(STRUCT, evaluate(structname));
	slice inner = structname;
	while(get(inner, -1) != '{')
		inner.begin++;
	inner.end = inner.begin;
	int len = strlen(inner.data);
	for(int i = inner.begin; i < len && data.data[i] != '}'; i++)
	{
		inner.end = i;
		if(get(inner, i - inner.begin) == ';')
		{
			ast_node *field = parse_vartype(inner);
			field->next = root->child;
			root->child = field;
			inner.begin = inner.end + 1;
		}
	}
	return root;
}

ast_node* parse_vartype(slice data)
{
	while(is_whitespace(get(data, 0)))
		data.begin++;
	slice varname;
	varname.data = data.data;
	varname.end = varname.begin = data.begin;
	int i = 0;
	while(get(varname, i) != ':') i++;
	varname.end += i;
	slice type;
	type.data = data.data;
	type.begin = varname.end + 1;
	type.end = data.end;
	char *varnameStr, *typeStr;
	varnameStr = evaluate(varname);
	typeStr = evaluate(type);
	ast_node *typeNode = new_node(TYPE, typeStr);
	ast_node *nameNode = new_node(VARTYPE, varnameStr);
	nameNode->child = typeNode;
	return nameNode;
}
