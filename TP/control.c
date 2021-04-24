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
		_tprintf(_T("ERRO! Já existe uma instância deste programa a correr!\n"));
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
		_tprintf(_T("\t-> Suspender/Ativar a aceitação de novos aviões (suspender <on/off>)\n"));
		_tprintf(_T("\t-> Listar todos os Aeroportos, Aviões e Passageiros (lista)\n"));
		_tprintf(_T("\t-> Encerrar (encerrar)\n"));
		_tprintf(_T("Comando: "));
		_fgetts(com, TAM, stdin);
		com[_tcslen(com) - 1] = '\0';
		_tprintf(_T("%s"),com);
		_tcstok(com,)

	} while (_tcscmp(com,_T("sair")) != 0);
/*
* A interface deve permitir receber os seguintes comandos:
	Encerrar todo o sistema (todas as aplicações são notificadas).
		Criar novos aeroportos.
		Suspender/ativar a aceitação de novos aviões por parte dos utilizadores.
		Listar todos os aeroportos, aviões e passageiros
*/


}
