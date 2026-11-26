// tm_parser.h
#ifndef TM_PARSER_H
#define TM_PARSER_H

#include "tm_lexer.h"

// Direção do cabeçote da máquina de Turing.
typedef enum
{
	MOVE_NONE,		// sem direção definida
	MOVE_LEFT,		// andar uma célula para a esquerda
	MOVE_RIGHT,		// andar uma célula para a direita
	MOVE_STOP,		// ficar parado na mesma célula
} MoveDirection;

typedef enum
{
    STATE_NORMAL,
    STATE_INITIAL,
    STATE_ACCEPT,
    STATE_REJECT
} StateType;

typedef struct
{
	StateType type;
	char name[MAX_LEXEMA];
} StateParam;

typedef struct
{
    char state[MAX_LEXEMA];
    char symbol;
} TaskParam;

typedef struct
{
    char tape[MAX_LEXEMA];
    char id[MAX_LEXEMA];
} TapeParam;

static const char* moveNames[] =
{
	"NO MOVE", "LEFT", "RIGHT", "STOP",
};

static const char* stateCondNames[] = 
{
	"NORMAL", "INITIAL", "ACCEPT", "REJECT",
};

typedef struct
{
    Action action;

    union
    {
        char id[MAX_LEXEMA];
        char symbol;
        MoveDirection dir;
        unsigned long number;
        StateParam state;
        TaskParam task;
        TapeParam tape;

    } param;

} Instruction;

// Lista dinâmica de instruções (o programa já analisado).
typedef struct
{
	Instruction* items;		// as instruções
	size_t count;			// quantas instruções existem
	size_t capacity;		// quantas cabem antes de crescer
	size_t errors;			// quantas linhas com erro foram encontradas
} InstList;

int 			TMReadLine(FILE* file, char** line, size_t *cap);								// lê uma linha do arquivo: 1 = leu, 0 = fim do arquivo, -1 = erro
int				TMReadFile(const char* path);													// lê o arquivo .tm, gera os tokens e os mostra: 1 = ok, 0 = erro

static size_t	TMLineOf(TokenIter* iter, size_t index);		// número da linha (a partir de 1) do token na posição index
static void		TMReportError(TokenIter* iter);					// mostra o erro da linha atual (léxico ou de sintaxe)
static void		TMSkipLine(TokenIter* iter);					// pula o resto da linha com erro para continuar a análise
static bool 	TMReadParam(TokenIter* iter,   Command expected,  char* dest,  size_t capacity);

static bool		TMParseBuild(TokenIter* iter, Instruction* inst);
static bool		TMParseState(TokenIter* iter, Instruction* inst);
static bool		TMParseWrite(TokenIter* iter, Instruction* inst);
static bool		TMParseMove(TokenIter* iter, Instruction* inst);
static bool		TMParseTask(TokenIter* iter, Instruction* inst);
static bool		TMParseTape(TokenIter* iter, Instruction* inst);
static bool		TMParseBlank(TokenIter* iter, Instruction* inst);
static bool		TMParseLimit(TokenIter* iter, Instruction* inst);

InstList		TMParseFunction(TokenIter* iter);				// percorre todos os tokens e monta a lista de instruções
void			TMGetInstructions(InstList list);				// imprime  toda a lista de instruções.

void			TMExecuteFunction(Instruction inst);			// executa uma instrução na máquina (a implementar)

#endif