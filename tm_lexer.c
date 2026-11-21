// tm_lexer.c
#include "tm_lexer.h"

Token	tokenize(Command type, const char* lex, size_t n, const char* base)
{
	Token tok = {0};
	
	tok.type 	= type;
	tok.col 	= (int)(lex - base) + 1; // Faz a diferença binária entre as strings do token.
	
	if(n >= MAX_LEXEMA)
	{
		tok.type = CMD_ERROR;
		tok.msg = "Lexema muito longo";
		n = MAX_LEXEMA - 1;
	}
	
	memcpy(tok.data, lex, n);
	tok.data[n] = '\0';
	
	return tok;
}

Token	tokenizeError(const char* msg, const char *lex, size_t n, const char* base)
{
	Token tokErr = tokenize(CMD_ERROR, lex, n, base);
	tokErr.msg = msg;
	return tokErr;
}

const char*	tokenFindClosure(const char *w, char quote)
{
	for(; *w != '\0' && *w != '\n'; w++)
	{
		if(*w == quote)
			return w;
	}
	
	return NULL;
}

Command		tokenClassify(const char *word)
{
	for(size_t i = 0; i < RESERVED_SIZE; i++)
		if(strcmp(word, RESERVED[i].data) == 0) // Verifica se as duas string são iguais.
			return RESERVED[i].type;
	
	return CMD_ID; // É identificador se não for palavra chave.
}

//	Retorna próximo token através do ponteiro da linha.
Token		tokenNext(const char** curr, const char* base)
{
	const char* p = *curr; // Ponteiro para iteração.
	const char* start; // Constante da linha inicial.
	
	Token tok;
	
	while(*p == ' ' || *p == '\t' || *p == '\r') p++; // Pula todos os espaços.
	
	start = p;

	if (*p == '\0')
	{
		tok = tokenize(CMD_EOF, p, 0, base);
	}
	else if (*p == '\n')
	{
		p++; // vá para o próximo caractere.
		tok = tokenize(CMD_NL, start, 0, base);
	}		
	else if (*p == '#')
	{
		while(*p != '\0'&& *p != '\n') p++; // pula todos os caracteres, menos o final e o pula-linha.
		*curr = p;
		return tokenNext(curr, base);
	}			
	else if (*p == ',')
	{
		p++;
		tok = tokenize(CMD_COMMA, start, 1, base);
	}
	else if(IS_ALPHA(*p))
	{
		while(IS_IDCHAR(*p))	p++;
		tok = tokenize(CMD_ID, start, (size_t)(p - start), base); // 
		
		if(tok.type != CMD_ERROR)
			tok.type = tokenClassify(tok.data);
	}
	else if(IS_DIGIT(*p))
	{
		while(IS_DIGIT(*p)) p++; 
		
		if(IS_IDCHAR(*p))
		{
			while(IS_IDCHAR(*p)) p++;
			tok = tokenizeError("Identicador inicia com dígito.", start, (size_t)(p - start), base);
		}
		else
		{
			tok = tokenize(CMD_NUMBER, start, (size_t)(p - start), base);
		}
	}
	else if(*p == '\'') // Exatamente 1 caractere.
	{
		if(p[1] == '\0' || p[1] == '\n')
		{
			p += 1;
			tok = tokenizeError("Caractere não foi fechado.", start, 1, base);
		}
		else if(p[1] == '\'') // caractere vazio.
		{
			p += 2;
			tok = tokenizeError("Símbolo está vazio.", start, 2, base);
		}
		else if(p[2] == '\'') // caractere preenchido.
		{
			p += 3;
			tok = tokenize(CMD_CHAR, start + 1, 1, base);
			tok.col = (int)(start - base) + 1;
		}
		else
		{
			const char *q = tokenFindClosure(p + 1, '\'');
			if(q != NULL)
			{
				p = q + 1;
				tok = tokenizeError("Símbolo deve ser somente um caractere.", start, (size_t)(p - start), base);
			}
			else
			{
				while(*p != '\0' && *p != '\n') p++;
				tok = tokenizeError("String não foi fechado.", start, (size_t)(p - start), base);
			}
		}
	}
	else if (*p == '"') // string.
	{
		const char *q = tokenFindClosure(p + 1, '"');
		
		if(q == NULL)
		{
			while(*p != '\0' && *p != '\n') p++;
			tok = tokenizeError("Símbolo não foi fechado.", start, (size_t)(p - start), base);
		}
		else
		{
			tok = tokenize(CMD_STRING, start + 1, (size_t)(q - start - 1), base);
			tok.col = (int)(start - base) + 1;
			p = q + 1;
		}
	}
	else
	{
		p++;
		tok = tokenizeError("Caractere Inesperado.", start, 1, base);
	}
	
	*curr = p;
	
	return tok;
}

void		tokenPrint(Token tok)
{
	if(tok.type == CMD_ERROR)
		printf("%-3zu [ERROR] : '%s' -> %s\n", tok.col, tok.data, tok.msg);
	else
		printf("%-3zu %s : %s\n", tok.col, tokNames[tok.type], tok.data);
}

void		tokenArrPrint(TokenArray tokArr)
{
	for(size_t i = 0; i < tokArr.count; i++)
		tokenPrint(tokArr.items[i]);
	printf("\n");
}

char*		tokenArrToStr(TokenArray tokArr)
{
	size_t size = 1;
	
	for(size_t i = 0; i < tokArr.count; i++)
        size += strlen(tokArr.items[i].data);

    char* str = malloc(size);

    if(str == NULL)
        return NULL;

    str[0] = '\0';

    for(size_t i = 0; i < tokArr.count; i++)
    {
        if(tokArr.items[i].type == CMD_EOF)
            break;

        strcat(str, tokArr.items[i].data);
    }

    return str;
}


void			tokenGetError(Token *tok, const char *msg)
{
	tok->type = CMD_ERROR;
    tok->msg = msg;

    tokenPrint(*tok);
}

Token* tokenPeek(TokenIter* iter)
{
    if(iter->index >= iter->arr->count)
        return NULL;

    return &iter->arr->items[iter->index];
}


Token* tokenConsume(TokenIter* iter)
{
    Token* tok = tokenPeek(iter);

    if(tok != NULL)
        iter->index++;

    return tok;
}

bool	tokenMatch(TokenIter* iter, Command type)
{
	Token* tok = tokenPeek(iter);
	
	if(tok == NULL || tok->type != type)
		return false;
	
	tokenConsume(iter);
	
	return true;
}

Action			commandToAction(Command command)
{
	for(size_t i = 0; i < RESV_ACTIONS_SIZE; i++)
		if(command == ReservedAction[i].type)
			return ReservedAction[i].action;
	
	return ACTION_NONE;
}



void		TMLexer(TokenArray *tokArr, const char* line)
{
	const char* curr = line;
	
	while(true)
	{
		Token nextTok = tokenNext(&curr, line);
		
		if(nextTok.type == CMD_EOF)
			break;
		
		da_append(tokArr, nextTok);
	}
}

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

static bool		TMCheckLineEnd(TokenIter* iter)
{
	Token* tok = tokenPeek(iter);
    return tok == NULL || tok->type == CMD_NL || tok->type == CMD_EOF;
}

static bool		TMCheckId(TokenIter* iter)
{
	return tokenMatch(iter, CMD_ID);
}

static bool		TMCheckNumber(TokenIter* iter)
{
	return tokenMatch(iter, CMD_NUMBER);
}

static bool		TMCheckChar(TokenIter* iter)
{
	return tokenMatch(iter, CMD_CHAR);
}

static bool		TMCheckString(TokenIter* iter)
{
	return tokenMatch(iter, CMD_STRING);
}



static inline bool	TMCheckBuild(TokenIter 	*iter)
{
	size_t start = iter->index;

    if(!tokenMatch(iter, CMD_BUILD))
        goto error;

    if(!TMCheckId(iter))
        goto error;

    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}
static inline bool	TMCheckEndBuild(TokenIter	*iter)
{
	size_t start = iter->index;

    if(!tokenMatch(iter, CMD_ENDBUILD))
        goto error;
	
    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}


static inline bool TMCheckState(TokenIter* iter)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_STATE))
        goto error;

    if(!TMCheckId(iter))
        goto error;

    Token* tok = tokenPeek(iter);

    if(tok != NULL)
    {
        switch(tok->type)
        {
            case CMD_INITIAL:
            case CMD_ACCEPT:
            case CMD_REJECT:
                tokenConsume(iter);
                break;

            default:
                break;
        }
    }

    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}

static inline bool TMCheckTask(TokenIter* iter)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_TASK))
        goto error;

    if(!TMCheckId(iter))
        goto error;

    if(!tokenMatch(iter, CMD_COMMA))
        goto error;

    if(!TMCheckChar(iter))
        goto error;

    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}
static inline bool	TMCheckEndTask(TokenIter	*iter)
{
	size_t start = iter->index;

    if(!tokenMatch(iter, CMD_ENDTASK))
        goto error;
	
    if(!TMCheckId(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}

static inline bool TMCheckWrite(TokenIter* iter)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_WRITE))
        goto error;

    if(!TMCheckChar(iter))
        goto error;

    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error: 
		iter->index = start;
		return false;
}

static inline bool TMCheckMove(TokenIter* iter)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_MOVE))
        goto error;

    Token* tok = tokenConsume(iter);

    if(tok == NULL)
        goto error;

    if(tok->type > CMD_STOP)
		goto error;

    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}

static inline bool TMCheckBlank(TokenIter* iter)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_BLANK))
        goto error;
	
    if(!TMCheckChar(iter))
        goto error;

    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}

static inline bool TMCheckTape(TokenIter* iter)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_TAPE))
        goto error;

    if(!TMCheckString(iter))
        goto error;

    if(!tokenMatch(iter, CMD_INTO))
        goto error;

    if(!TMCheckId(iter))
        goto error;

    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}


static inline bool TMCheckRun(TokenIter* iter)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_RUN))    goto error;
    if(!TMCheckLineEnd(iter))	      goto error;

    return true;

	error:
		iter->index = start;
		return false;
}

static inline bool TMCheckLimit(TokenIter* iter)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_LIMIT))
        goto error;

    if(!TMCheckNumber(iter))
        goto error;

    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}

static inline bool TMCheckCheck(TokenIter* iter)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_CHECK))
        goto error;

    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}

static inline bool TMCheckHalt(TokenIter* iter)
{
    size_t start = iter->index;

    if(!tokenMatch(iter, CMD_HALT))
        goto error;

    if(!TMCheckLineEnd(iter))
        goto error;

    return true;

	error:
		iter->index = start;
		return false;
}



Action TMCheckFunction(TokenIter* iter)
{
    Token* tok = tokenPeek(iter);

    if(tok == NULL)
        return ACTION_NONE;

    // Ignora linhas vazias.
    if(tok->type == CMD_NL)
    {
        tokenConsume(iter);
        return ACTION_NONE;
    }

    // Trata erros encontrados pelo lexer.
    if(tok->type == CMD_ERROR)
        return ACTION_ERROR;

    Action action = commandToAction(tok->type);

    bool valid = false;

	// Checar se o token possuem uma dessas ações.
    switch(action)
    {
        case ACTION_BUILD:
            valid = TMCheckBuild(iter);
            break;

        case ACTION_STATE:
            valid = TMCheckState(iter);
            break;

        case ACTION_TASK:
            valid = TMCheckTask(iter);
            break;

        case ACTION_WRITE:
            valid = TMCheckWrite(iter);
            break;

        case ACTION_MOVE:
            valid = TMCheckMove(iter);
            break;

        case ACTION_BLANK:
            valid = TMCheckBlank(iter);
            break;

        case ACTION_TAPE:
            valid = TMCheckTape(iter);
            break;

        case ACTION_RUN:
            valid = TMCheckRun(iter);
            break;

        case ACTION_LIMIT:
            valid = TMCheckLimit(iter);
            break;

        case ACTION_CHECK:
            valid = TMCheckCheck(iter);
            break;

        case ACTION_HALT:
            valid = TMCheckHalt(iter);
            break;

        default:
            return ACTION_ERROR;
    }

    if(!valid)
        return ACTION_ERROR;

    // Consome o delimitador da instrução.
    tokenMatch(iter, CMD_NL);

    return action;
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

        printf("[INS %zu] : %s", i, actionNames[ins.action]);

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
            case ACTION_BLANK:
                printf(" | symbol: %c", ins.param.symbol);
                break;

            case ACTION_MOVE:
                printf(" | direction: %s", moveNames[ins.param.dir]);
                break;

            case ACTION_TAPE:
                printf(" | tape: %s | id: %s", ins.param.tape.tape, ins.param.tape.id);
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


int main(int argc, char **argv)
{
	if(argc > 1)
	{
		if(!TMReadFile(argv[1]))
			return 1;
	}
	else
	{
		TokenArray arr = (TokenArray){0};
		
		TokenIter iter = {0};
		InstList list = {0};
		
		puts("===== Tokenização por cursor =====\n");
		TMLexer(&arr, "task q0, ','\n");
		TMLexer(&arr, "tape \"10 11\" into incremento\n");
		TMLexer(&arr, "limit 1000 # comentario\n");
		TMLexer(&arr, "task q0, '#'\n");
		TMLexer(&arr, "blank '_'\0");
		
		// tokenArrPrint(arr);
		
		iter.arr = &arr;
		iter.index = 0;
		
		list = TMParseFunction(&iter);
		TMGetInstructions(list);
		
		da_free(&arr);
		
		puts("===== Erros viram tokens CMD_ERROR =====\n");
		TMLexer(&arr, "12ab\n");
		TMLexer(&arr, "write 'ab'\n");
		TMLexer(&arr, "write ''\n");
		TMLexer(&arr, "write 'a\n");
		TMLexer(&arr, "blank = '_'\n");
		TMLexer(&arr, "tape \"10\n");
		TMLexer(&arr, "state a@b initial\0");
		
		// tokenArrPrint(arr);
		
		iter.arr = &arr;
		iter.index = 0;
		
		list = TMParseFunction(&iter);
		TMGetInstructions(list);
		
		da_free(&arr);
	}
	
	return 0;
}