// tm_lexer.h

#pragma once

// ---------- Bibliotecas usadas ----------
#include <stdio.h>		
#include <stdlib.h>		
#include <assert.h>		
#include <stdbool.h>	
#include <string.h>			// para usar: strcmp, strcpy, memcpy, strlen
#include <ctype.h>			// para usar: isalpha, isdigit, isalnum
#include <sys/stat.h>		// para usar: stat (usado em pathIsDir)

// ---------- Tokens ----------

/* 	
	Tipos de token (Command).
	Cada pedaço do texto lido vira um token com um destes tipos:
	CMD_EOF    = fim do texto            CMD_NL     = quebra de linha
	CMD_NUMBER = número (1000)           CMD_ID     = nome de estado/autômato (q0)
	CMD_CHAR   = símbolo ('a')           CMD_STRING = fita ("1011")
	CMD_EQUAL  = "="                     CMD_COMMA  = ","
	CMD_BUILD ... CMD_HALT = palavras reservadas da linguagem
	CMD_ERROR  = texto inválido (a mensagem fica em Token.msg)
*/
typedef enum
{
	CMD_EOF,		CMD_NL,			CMD_NUMBER,		CMD_ID,			CMD_CHAR,		
	CMD_STRING,		CMD_COMMA,		CMD_BUILD,		CMD_ENDBUILD,	CMD_STATE,		
	CMD_TASK,		CMD_ENDTASK,	CMD_WRITE,		CMD_MOVE,		CMD_BLANK,		
	CMD_TAPE,		CMD_INTO,		CMD_RUN,		CMD_LIMIT,		CMD_INITIAL,	
	CMD_ACCEPT,		CMD_REJECT,		CMD_CHECK,		CMD_LEFT,		CMD_RIGHT,		
	CMD_STOP,		CMD_HALT,		CMD_ERROR,
} Command;

// Nome de cada tipo de token, usado para imprimir.
// Precisa estar na mesma ordem do enum Command (o valor do enum é o índice).
static const char* tokNames[] =
{
	"EOF", 		"NL", 		"NUMBER", 	"ID", 		"CHAR", 
	"STRING",	"COMMA", 	"BUILD", 	"ENDBUILD", "STATE", 	
	"TASK",		"ENDTASK", 	"WRITE", 	"MOVE", 	"BLANK", 	
	"TAPE", 	"INTO",		"RUN", 		"LIMIT",	"INITIAL", 	
	"ACCEPT", 	"REJECT", 	"CHECK",	"LEFT", 	"RIGHT", 	
	"STOP",		"HALT",		"ERROR",
};

// Tabela de palavras reservadas: texto escrito no código -> tipo de token.
// L, R e S são atalhos de left, right e stop.
static const struct { const char* data; Command type;} RESERVED[] =
{
	{"build", 		CMD_BUILD},
	{"endbuild", 	CMD_ENDBUILD},
	{"state", 		CMD_STATE},
	{"task", 		CMD_TASK},
	{"endtask", 	CMD_ENDTASK},
	{"write", 		CMD_WRITE},
	{"move", 		CMD_MOVE},
	{"blank", 		CMD_BLANK},
	{"tape", 		CMD_TAPE},
	{"into", 		CMD_INTO},
	{"run", 		CMD_RUN},
	{"limit", 		CMD_LIMIT},
	{"initial", 	CMD_INITIAL},
	{"accept", 		CMD_ACCEPT},
	{"reject", 		CMD_REJECT},
	{"check", 		CMD_CHECK},
	{"left", 		CMD_LEFT},
	{"L", 			CMD_LEFT},		// atalho de left
	{"right", 		CMD_RIGHT},
	{"R", 			CMD_RIGHT},		// atalho de right
	{"stop", 		CMD_STOP},
	{"S", 			CMD_STOP},		// atalho de stop
	{"halt", 		CMD_HALT},
};

// Quantidade de palavras reservadas (tamanho da tabela RESERVED).
static const size_t RESERVED_SIZE = sizeof(RESERVED) / sizeof(RESERVED[0]);

// Tamanho máximo do texto de um token (63 caracteres + o '\0' do fim).
#define MAX_LEXEMA 64

// Um token: o menor pedaço com significado do texto (palavra, número, símbolo...).
typedef struct
{
	Command type;				// tipo do token
	char	data[MAX_LEXEMA];	// texto do token (ex.: "state", "q0")
	const 	char*	msg;		// mensagem de erro (só usada quando type == CMD_ERROR)
	size_t 	col;				// coluna onde o token começa (1 = primeira da linha) 
} Token;

// ---------- Arranjo dinâmico (lista que cresce sozinha) ----------

// Capacidade inicial do arranjo, em número de elementos.
#define INIT_CAP 8

/* 
	Adiciona o elemento x no fim do arranjo dinâmico arr.
	O arranjo precisa ter os campos items, count e capacity (ex.: TokenArray).
	Se estiver cheio, a capacidade dobra (começa em INIT_CAP) usando realloc.
	Baseado nas implementações de Tsoding https://www.youtube.com/@Tsoding.
*/
#define da_append(arr, x)																	\
	do{																						\
		if((arr)->count >= (arr)->capacity){												\
			if((arr)->capacity == 0) 	(arr)->capacity = INIT_CAP;							\
			else					(arr)->capacity *= 2;									\
			(arr)->items = realloc((arr)->items, (arr)->capacity * sizeof(*(arr)->items));	\
			assert((arr)->items != NULL && "Falta de Memória");								\
		}																					\
		(arr)->items[(arr)->count++] = (x);													\
	} while(0)																	
		

// Libera a memória do arranjo e o deixa vazio, pronto para ser usado de novo.
#define da_free(arr) 				\
	do{								\
		free((arr)->items);			\
		(arr)->items 	= NULL;		\
		(arr)->count 	= 0;		\
		(arr)->capacity = 0;		\
	} while(0)

// Lista dinâmica de tokens (todos os tokens do programa).
typedef struct
{
	Token *items;		// os tokens
	size_t count;		// quantos tokens existem
	size_t capacity;		// quantos cabem antes de crescer
} TokenArray;

// Iterador: "dedo" que anda pelos tokens, um de cada vez, durante a checagem (parser).
typedef struct
{
	TokenArray* arr;		// lista de tokens que está sendo percorrida
	size_t		index;		// posição do token atual
} TokenIter;

// ---------- Ações (o que cada instrução faz) ----------

// Ação que uma linha do programa representa.

// ACTION_NONE = linha vazia | ACTION_BUILD ... ACTION_HALT = uma ação por instrução
// ACTION_ERROR = instrução inválida
typedef enum
{
	ACTION_NONE, 		ACTION_BUILD,		ACTION_ENDBUILD, 	ACTION_STATE,		ACTION_TASK,		ACTION_ENDTASK,
	ACTION_WRITE,		ACTION_MOVE,		ACTION_BLANK,		ACTION_TAPE,		ACTION_RUN,			ACTION_LIMIT,		
	ACTION_CHECK,		ACTION_HALT,		ACTION_ERROR,
} Action;

static const char* actionNames[] =
{
	"NONE",		"BUILD", 	"ENDBUILD", "STATE", 	"TASK",		"ENDTASK",
	"WRITE", 	"MOVE", 	"BLANK", 	"TAPE", 	"RUN", 		"LIMIT",
	"CHECK",	"HALT",		"ERROR",
};

// Tabela: primeiro token da linha (Command) -> ação correspondente (Action).
static const struct { Command type; Action action; } ReservedAction[] =
{
	{CMD_BUILD,	 	ACTION_BUILD},
	{CMD_ENDBUILD, 	ACTION_ENDBUILD},
	{CMD_STATE, 	ACTION_STATE},
	{CMD_TASK, 		ACTION_TASK},
	{CMD_ENDTASK, 	ACTION_ENDTASK},
	{CMD_WRITE, 	ACTION_WRITE},
	{CMD_MOVE, 		ACTION_MOVE},
	{CMD_BLANK, 	ACTION_BLANK},
	{CMD_TAPE, 		ACTION_TAPE},
	{CMD_RUN, 		ACTION_RUN},
	{CMD_LIMIT, 	ACTION_LIMIT},
	{CMD_CHECK, 	ACTION_CHECK},
	{CMD_HALT, 		ACTION_HALT},
};

// Quantidade de linhas da tabela ReservedAction.
static const size_t RESV_ACTIONS_SIZE = sizeof(ReservedAction) / sizeof(ReservedAction[0]);

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

// ---------- Funções do lexer (texto -> tokens) ----------
Token			tokenize(Command type, const char* lex, size_t n, const char* base);	// cria token do tipo type com lex de tamanho de 'n' (base é usada para calcular coluna)
Token			tokenizeError(const char* msg, const char* lex, size_t n, const char* base);	// cria um token de erro (CMD_ERROR) com a mensagem msg
const char*		tokenFindClosure(const char *w, char quote);									// procura o fechamento das aspas (quote) na linha; devolve NULL se não achar
Command			tokenClassify(const char *word);												// palavra reservada: devolve o tipo; senão, CMD_ID
Token			tokenNext(const char** curr, const char* base);									// lê o próximo token da linha (curr avança; base é o início da linha)
void			tokenPrint(Token tok);															// imprime um token
void			tokenArrPrint(TokenArray tokArr);												// imprime todos os tokens do arranjo
void			tokenGetError(Token *tok, const char *msg);										// marca o token como erro, com a mensagem msg, e o imprime

// ---------- Funções do iterador (andar pelos tokens) ----------
Token* 			tokenPeek(TokenIter* iter);														// olha o token atual sem avançar (NULL se acabou)
Token* 			tokenConsume(TokenIter* iter);													// devolve o token atual e avança para o próximo
bool 			tokenMatch(TokenIter* iter, Command type);										// se o token atual for do tipo pedido, avança e devolve true; senão, false

Action			commandToAction(Command command);												// converte o tipo do primeiro token da linha na ação dela

// ---------- Leitura de arquivo ----------
void			TMLexer(TokenArray *tokArr, const char* line);									// divide uma linha em tokens e os guarda em tokArr
int 			TMReadLine(FILE* file, char** line, size_t *cap);								// lê uma linha do arquivo: 1 = leu, 0 = fim do arquivo, -1 = erro
int				TMReadFile(const char* path);													// lê o arquivo .tm, gera os tokens e os mostra: 1 = ok, 0 = erro

// ---------- Checagem de sintaxe: partes de uma instrução ----------
static bool		TMCheckLineEnd(TokenIter* iter);											// a linha terminou? (NL ou EOF)
static bool		TMCheckId(TokenIter* iter);													// espera um identificador (nome)
static bool		TMCheckNumber(TokenIter* iter);												// espera um número
static bool		TMCheckChar(TokenIter* iter);												// espera um símbolo entre aspas simples, ex.: '0'
static bool		TMCheckString(TokenIter* iter);												// espera uma fita entre aspas duplas, ex.: "1011"

// ---------- Checagem de sintaxe: uma função por instrução ----------
// Cada uma devolve true se a linha está correta (e avança), ou false (e volta ao início da linha).
static inline bool	TMCheckBuild(TokenIter 		*iter);		// build <nome>
static inline bool	TMCheckEndBuild(TokenIter	*iter);		// endbuild <NL>
static inline bool	TMCheckState(TokenIter 		*iter);		// state <nome> [initial | accept | reject]
static inline bool	TMCheckTask(TokenIter 		*iter);		// task <estado>, <símbolo>
static inline bool	TMCheckEndTask(TokenIter	*iter);		// endtask <estado>
static inline bool	TMCheckWrite(TokenIter 		*iter);		// write <símbolo>
static inline bool	TMCheckMove(TokenIter 		*iter);		// move <left | right | stop>
static inline bool	TMCheckBlank(TokenIter 		*iter);		// blank = <símbolo>
static inline bool	TMCheckTape(TokenIter 		*iter);		// tape "<fita>" into <nome>
static inline bool	TMCheckRun(TokenIter 		*iter);		// run <nome> [<fita>] [limit <número>]
static inline bool	TMCheckLimit(TokenIter 		*iter);		// limit <número>
static inline bool	TMCheckCheck(TokenIter 		*iter);		// check <nome>
static inline bool	TMCheckHalt(TokenIter 		*iter);		// halt

// ---------- Parser (tokens -> instruções) ----------
Action			TMCheckFunction(TokenIter* iter);				// confere uma linha e devolve a ação dela (ACTION_ERROR se estiver errada)
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

// ---------- Testes de caractere ----------
// O (unsigned char) evita erro com acentos. IS_IDCHAR = letra, dígito ou "_" (vale dentro de um nome).
#define IS_ALPHA(c)		isalpha((unsigned char)(c))
#define IS_DIGIT(c)		isdigit((unsigned char)(c))
#define IS_IDCHAR(c)	(isalnum((unsigned char)(c)) || (c) == '_')

// ---------- Arquivos ----------

// Usa stat para consultar o caminho: devolve false se ele não existe.
// Atenção: apesar do nome, hoje devolve true para qualquer caminho que exista (arquivo ou pasta).
static inline bool pathIsDir(const char* filePath) 
{
  struct stat st_buf;
  
  if(stat(filePath, &st_buf) != 0)
	  return false;

  return true;
}