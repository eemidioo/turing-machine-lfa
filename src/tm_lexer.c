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

bool		TMCheckLineEnd(TokenIter* iter)
{
	Token* tok = tokenPeek(iter);
    return tok == NULL || tok->type == CMD_NL || tok->type == CMD_EOF;
}

bool		TMCheckId(TokenIter* iter)
{
	return tokenMatch(iter, CMD_ID);
}

bool		TMCheckNumber(TokenIter* iter)
{
	return tokenMatch(iter, CMD_NUMBER);
}

bool		TMCheckChar(TokenIter* iter)
{
	return tokenMatch(iter, CMD_CHAR);
}

bool		TMCheckString(TokenIter* iter)
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