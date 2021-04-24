#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "estruturas.h"

void menu();

int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif


	CreateMutex(0, FALSE, "Local\\$controlador$"); // try to create a named mutex
	if (GetLastError() == ERROR_ALREADY_EXISTS) // did the mutex already exist?
	{
		_tprintf(_T("ERRO! J� existe uma inst�ncia deste programa a correr!\n"));
		return -1; // quit; mutex is released automatically
	}
	_tprintf(_T("--------------------- CONTROLADOR ---------------------\n"));


	menu();

	return 0;
}

void menu() {
	TCHAR com[TAM];
	TCHAR com_split[TAM];
	do {
		_tprintf(_T("Menu Principal\n"));
		_tprintf(_T("\t-> Criar novo Aeroporto (criar <nome> <posicaoX> <posicaoY>)\n"));
		_tprintf(_T("\t-> Suspender/Ativar a aceita��o de novos avi�es (suspender <on/off>)\n"));
		_tprintf(_T("\t-> Listar todos os Aeroportos, Avi�es e Passageiros (lista)\n"));
		_tprintf(_T("\t-> Encerrar (encerrar)\n"));
		_tprintf(_T("Comando: "));
		_fgetts(com, TAM, stdin);
		com[_tcslen(com) - 1] = '\0';
		_tprintf(_T("%s"),com);
		_tcstok(com,)

	} while (_tcscmp(com,_T("sair")) != 0);
/*
* A interface deve permitir receber os seguintes comandos:
	Encerrar todo o sistema (todas as aplica��es s�o notificadas).
		Criar novos aeroportos.
		Suspender/ativar a aceita��o de novos avi�es por parte dos utilizadores.
		Listar todos os aeroportos, avi�es e passageiros
*/


}
