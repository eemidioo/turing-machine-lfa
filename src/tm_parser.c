// tm_parser.c
#include "tm_parser.h"

int 	TMReadLine(FILE* file, char** line, size_t *cap)
{
	size_t len = 0;
	int c;
	
	if(*line == NULL || *cap == 0)
	{
		*cap = 128;
		*line = malloc(*cap);
		
		if(*line == NULL)
			return -1;
	}
	
	while((c = fgetc(file)) != EOF)
	{
		if(len + 1 >= *cap)
		{
			size_t new_cap = *cap * 2;
			char* temp = realloc(*line, new_cap);
			
			if(temp == NULL) return -1;
			
			*line = temp;
			*cap = new_cap;
		}
		
		(*line)[len++] = (char)c;
		
		if(c == '\n')
			break;
	}
	
	if(ferror(file))
		return -1;
	
	if(len == 0)
		return 0;
	
	(*line)[len] = '\0';
	
	return 1;
}

int		TMReadFile(const char* path)
{
	if(!pathIsDir(path))
		return 0;
	
	FILE* tmf = fopen(path, "r");
	
	if(tmf == NULL)
	{
		perror(path);
		return 0;
	}
	
	char *line = NULL;
	size_t cap = 0;
	int result = 0;
	
	TokenArray tokArr = (TokenArray){0};
	
	while((result = TMReadLine(tmf, &line, &cap)) == 1)
		TMLexer(&tokArr, line);
	
	Token tokEOF = tokenize(CMD_EOF, "\0", 0, "\0");
	
	da_append(&tokArr, tokEOF);
	
	
	TokenIter iter = {.arr = &tokArr, .index = 0};
	InstList list = TMParseFunction(&iter);
	TMGetInstructions(list);
	
	free(line);
	fclose(tmf);
	da_free(&tokArr);
	
	if(result == -1)
	{
		fprintf(stderr, "Erro na leitura do arquivo.\n");
		return 0;
	}
	
	return 1;
}

static size_t	TMLineOf(TokenIter* iter, size_t index)
{
	size_t line = 1;

	for(size_t i = 0; i < index && i < iter->arr->count; i++)
		if(iter->arr->items[i].type == CMD_NL)
			line++;

	return line;
}	

static void		TMReportError(TokenIter* iter)
{
	size_t line = TMLineOf(iter, iter->index);

	for(size_t i = iter->index; i < iter->arr->count; i++)
	{
		Token* t = &iter->arr->items[i];

		if(t->type == CMD_NL || t->type == CMD_EOF)
			break;

		if(t->type == CMD_ERROR)
		{
			fprintf(stderr, "Erro léxico (linha %zu, coluna %zu): '%s' -> %s\n", line, t->col, t->data, t->msg);
			return;
		}
	}

	Token* first = tokenPeek(iter);

	if(first != NULL)
		fprintf(stderr, "Erro de sintaxe (linha %zu, coluna %zu): instrução inválida iniciada em '%s'\n", line, first->col, first->data);
}
	
static void		TMSkipLine(TokenIter* iter)
{
	Token* t;

	while((t = tokenPeek(iter)) != NULL && t->type != CMD_NL && t->type != CMD_EOF)
		tokenConsume(iter);

	tokenMatch(iter, CMD_NL);
}

static bool TMReadParam(TokenIter* iter,   Command expected,  char* dest,  size_t capacity)
{
    Token* tok = tokenPeek(iter);

    if(tok == NULL || tok->type != expected)
        return false;

    if(strlen(tok->data) >= capacity)
        return false;

    strcpy(dest, tok->data);

    tokenConsume(iter);

    return true;
}

static bool TMFinishInstruction(TokenIter* iter)
{
    Token* tok = tokenPeek(iter);

    return tok == NULL ||  tok->type == CMD_NL || tok->type == CMD_EOF;
}

static bool		TMParseBuild(TokenIter* iter, Instruction* inst)
{
	size_t start = iter->index;

    if(!tokenMatch(iter, CMD_BUILD))        								goto error;
    if(!TMReadParam(iter, CMD_ID, inst->param.id, sizeof(inst->param.id)))  goto error;
    if(!TMFinishInstruction(iter))									        goto error;

    inst->action = ACTION_BUILD;

    return true;

	error:
		iter->index = start;
		return false;
}

static bool TMParseEndBuild(TokenIter* iter, Instruction* inst)
{
    Token* tok = tokenPeek(iter);

    if(tok == NULL || tok->type != CMD_ENDBUILD)
        return false;

    tokenConsume(iter);

    inst->action = ACTION_ENDBUILD;

    return TMCheckLineEnd(iter);
}

static bool TMParseState(TokenIter* iter, Instruction* inst)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_STATE))       									goto error;
	
	Token *tokName = tokenConsume(iter);
	
    if(tokName == NULL || tokName->type != CMD_ID) 	goto error;

    inst->param.state.type = STATE_NORMAL;

    Token* tokCond = tokenPeek(iter);

    if(tokCond != NULL)
    {
        switch(tokCond->type)
        {
            case CMD_INITIAL:
                inst->param.state.type = STATE_INITIAL;
                tokenConsume(iter);
                break;

            case CMD_ACCEPT:
                inst->param.state.type = STATE_ACCEPT;
                tokenConsume(iter);
                break;

            case CMD_REJECT:
                inst->param.state.type = STATE_REJECT;
                tokenConsume(iter);
                break;

            default:
                break;
        }
    }

    if(!TMFinishInstruction(iter))   goto error;
    inst->action = ACTION_STATE;
	strcpy(inst->param.state.name, tokName->data);
	
    return true;

	error:
		iter->index = start;
		return false;
}

static bool		TMParseWrite(TokenIter* iter, Instruction* inst)
{
	size_t start = iter->index;

    if(!tokenMatch(iter, CMD_WRITE))
        goto error;

    Token* tok = tokenPeek(iter);
    if(tok == NULL || (tok->type != CMD_CHAR && tok->type != CMD_BLANK))        goto error;
    inst->param.symbol = tok->data[0];
    tokenConsume(iter);

    if(!TMFinishInstruction(iter))        		goto error;
    inst->action = ACTION_WRITE;
    return true;

	error:
		iter->index = start;
		return false;
}

static bool TMParseMove(TokenIter* iter, Instruction* inst)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_MOVE)) 	goto error;
    Token* tok = tokenConsume(iter);
    if(tok == NULL)      		 		goto error;

    switch(tok->type)
    {
        case CMD_LEFT:
            inst->param.dir = MOVE_LEFT;
            break;

        case CMD_RIGHT:
            inst->param.dir = MOVE_RIGHT;
            break;

        case CMD_STOP:
            inst->param.dir = MOVE_STOP;
            break;

        default:
            goto error;
    }

    if(!TMFinishInstruction(iter))      goto error;
    inst->action = ACTION_MOVE;
    return true;

	error:
		iter->index = start;
		return false;
}

static bool		TMParseTask(TokenIter* iter, Instruction* inst)
{
	size_t start = iter->index;

    if(!tokenMatch(iter, CMD_TASK)) 						goto error;
	
	Token* tokState = tokenConsume(iter);
	if(tokState == NULL || tokState->type != CMD_ID) 		goto error;

    if(!tokenMatch(iter, CMD_COMMA))						goto error;
	
	Token* tokSymbol = tokenConsume(iter);
    if(tokSymbol == NULL || (tokSymbol->type != CMD_CHAR && tokSymbol->type != CMD_BLANK))	goto error;
	
    if(!TMFinishInstruction(iter))							goto error;
	
	inst->action 	= ACTION_TASK;
	strcpy(inst->param.task.state, tokState->data);
	inst->param.task.symbol 	= tokSymbol->data[0];

    return true;

	error:
		iter->index = start;
		return false;
	
}

static bool TMParseEndTask(TokenIter* iter, Instruction* inst)
{
    Token* tok = tokenPeek(iter);

    if(tok == NULL || tok->type != CMD_ENDTASK)
        return false;

    tokenConsume(iter);

    if(!TMReadParam(iter, CMD_ID, inst->param.id, MAX_LEXEMA))
        return false;

    inst->action = ACTION_ENDTASK;

    return TMCheckLineEnd(iter);
}

static bool		TMParseTape(TokenIter* iter, Instruction* inst)
{
	size_t start = iter->index;

    if(!tokenMatch(iter, CMD_TAPE))       		  	 goto error;
	
	Token* tokStr = tokenConsume(iter);
    if(tokStr == NULL || tokStr->type != CMD_STRING) goto error;

    if(!tokenMatch(iter, CMD_INTO))        			 goto error;
	
	Token* tokId = tokenConsume(iter);
    if(tokId == NULL || tokId->type != CMD_ID)       goto error;

    if(!TMFinishInstruction(iter))        			 goto error;

	inst->action = ACTION_TAPE;
	
	strcpy(inst->param.tape.tape, tokStr->data);
	strcpy(inst->param.tape.id, tokId->data);

    return true;

	error:
		iter->index = start;
		return false;
}

static bool		TMParseBlank(TokenIter* iter, Instruction* inst)
{
	size_t start = iter->index;
	
    if(!tokenMatch(iter, CMD_BLANK))  goto error;
	
	Token* tokChar = tokenConsume(iter);
    if(tokChar == NULL || tokChar->type != CMD_CHAR)     goto error;

    if(!TMFinishInstruction(iter))     					 goto error;

	inst->action = ACTION_BLANK;
	inst->param.symbol = tokChar->data[0];
	
    return true;

	error:
		iter->index = start;
		return false;
}

static bool		TMParseLimit(TokenIter* iter, Instruction* inst)
{
	size_t start = iter->index;

    if(!tokenMatch(iter, CMD_LIMIT))    				goto error;

	Token* tokNum = tokenConsume(iter);
    if(tokNum == NULL || tokNum->type != CMD_NUMBER) 	goto error;

    if(!TMFinishInstruction(iter))      				goto error;

	inst->action = ACTION_LIMIT;
	inst->param.number = strtoul(tokNum->data, NULL, 10);

    return true;

	error:
		iter->index = start;
		return false;
}

void TMGetInstructions(InstList list)
{
    for(size_t i = 0; i < list.count; i++)
    {
        Instruction ins = list.items[i];

        printf("[INS %zu] : %s", i + 1, actionNames[ins.action]);

        switch(ins.action)
        {
            case ACTION_BUILD:
                printf(" | id: %s", ins.param.id);
                break;
			case ACTION_ENDBUILD:
				printf("\n");
				break;
				
            case ACTION_STATE:
                printf(" | name: %s | id: %s", ins.param.state.name, stateCondNames[ins.param.state.type]);
                break;

            case ACTION_TASK:
                printf(" | state: %s | symbol: %c", ins.param.task.state, ins.param.task.symbol);
                break;
			case ACTION_ENDTASK:
				printf(" next state = %s\n", ins.param.id);
				break;
            case ACTION_WRITE:
				printf(" | write symbol : %c", ins.param.symbol);
				break;
            case ACTION_BLANK:
                printf(" | symbol: %c", ins.param.symbol);
                break;

            case ACTION_MOVE:
                printf(" | direction: %s", moveNames[ins.param.dir]);
                break;

            case ACTION_TAPE:
                printf(" | tape: %s | build id: %s", ins.param.tape.tape, ins.param.tape.id);
                break;

            case ACTION_LIMIT:
                printf(" | limit: %lu", ins.param.number);
                break;

            default:
                break;
        }

        printf("\n");
    }
}

	
InstList TMParseFunction(TokenIter* iter)
{
    InstList list = {0};

    while(tokenPeek(iter) != NULL && tokenPeek(iter)->type != CMD_EOF)
    {
        Instruction inst = {0};

        Token* tok = tokenPeek(iter);

        if(tok->type == CMD_NL)
        {
            tokenConsume(iter);
            continue;
        }

        if(tok->type == CMD_ERROR)
        {
            TMReportError(iter);
            TMSkipLine(iter);
            list.errors++;
            continue;
        }

        Action action = commandToAction(tok->type);
        bool valid = false;

        switch(action)
        {
            case ACTION_BUILD:
                valid = TMParseBuild(iter, &inst);
                break;
				
			case ACTION_ENDBUILD:
                valid = TMParseEndBuild(iter, &inst);
                break;
			
            case ACTION_STATE:
                valid = TMParseState(iter, &inst);
                break;

            case ACTION_TASK:
                valid = TMParseTask(iter, &inst);
                break;
			
			case ACTION_ENDTASK:
				valid = TMParseEndTask(iter, &inst);
				break;
				
            case ACTION_WRITE:
                valid = TMParseWrite(iter, &inst);
                break;

            case ACTION_MOVE:
                valid = TMParseMove(iter, &inst);
                break;

            case ACTION_TAPE:
                valid = TMParseTape(iter, &inst);
                break;

            case ACTION_BLANK:
                valid = TMParseBlank(iter, &inst);
                break;

            case ACTION_LIMIT:
                valid = TMParseLimit(iter, &inst);
                break;

            default:
                valid = false;
                break;
        }

        if(!valid)
        {
            TMReportError(iter);
            TMSkipLine(iter);
            list.errors++;
            continue;
        }

        tokenMatch(iter, CMD_NL);

        da_append(&list, inst);
    }

    return list;
}

void			TMExecuteFunction(Instruction inst)
{
	/*
		Executa as funções através da instrução proposta.
	*/
	
	(void)inst;
}