#include "src/tm_lexer.h"
#include "src/tm_parser.h"

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