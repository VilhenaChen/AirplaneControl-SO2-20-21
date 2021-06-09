#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "../Control/estruturas.h"

typedef struct {
	struct_passageiro eu;
} struct_dados;

int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif
	
	// recebe origem destino nome tempo_espera(opcional)
	// Encerra quando chegar ao destino ou quando passar o tempo maximo para embarque
	// Pode desligar-se a qualquer momento, deixando de existir
	// Comunica apenas e so com o controlador atraves de named Pipes
	struct_dados dados;
	struct_aeroporto origem;
	struct_aeroporto destino;
	dados.eu.aviao = NULL;
	dados.eu.origem = &origem;
	_tsccpy_s(dados.eu.origem->nome, _countof(dados.eu.origem->nome), argv[1]);
	dados.eu.destino = &destino;
	_tsccpy_s(dados.eu.destino->nome, _countof(dados.eu.destino->nome), argv[2]);
	_tsccpy_s(dados.eu.nome, _countof(dados.eu.nome), argv[3]);
	dados.eu.tempo_espera = _tstoi(argv[4]);


	return 0;
}