#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "estruturas.h"


//Declaracao de Funcoes e Threads
DWORD WINAPI Menu(LPVOID param);


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


//Codigo de Funcoes e Threads
DWORD WINAPI Menu(LPVOID param) {
	TCHAR com_total[TAM];
	TCHAR com[TAM], arg1[TAM],arg2[TAM],arg3[TAM];
	int cont;
	do {
		com_total[0] = '\0';
		com[0] = '\0';
		arg1[0] = '\0';
		arg2[0] = '\0';
		arg3[0] = '\0';
		cont = 0;
		_tprintf(_T("Menu Principal\n"));
		_tprintf(_T("\t-> Criar novo Aeroporto (criar <nome> <posicaoX> <posicaoY>)\n"));
		_tprintf(_T("\t-> Suspender/Ativar a aceitação de novos aviões (suspender <on/off>)\n"));
		_tprintf(_T("\t-> Listar todos os Aeroportos, Aviões e Passageiros (lista)\n"));
		_tprintf(_T("\t-> Encerrar (encerrar)\n"));
		_tprintf(_T("Comando: "));
		_fgetts(com_total, TAM, stdin);
		com_total[_tcslen(com_total) - 1] = '\0';
		cont = _stscanf_s(com_total, _T("%s %s %s %s"), com, (unsigned)_countof(com), arg1,(unsigned)_countof(arg1), arg2,(unsigned)_countof(arg2), arg3,(unsigned)_countof(arg3));
		if (_tcscmp(com, _T("criar")) == 0 && cont == 4) {
			
		}
		else {
			if (_tcscmp(com, _T("suspender")) == 0 && cont == 2) {

			}
			else {
				if (_tcscmp(com, _T("lista")) == 0) {

				}
				else {
					if (_tcscmp(com, _T("encerrar")) == 0) {
						break;
					}
					else {
						_tprintf(_T("Comando Inválido!!!!\n"));
									fflush(stdin);
					}
				}
			}
		}

	} while (TRUE);
		/*
		* A interface deve permitir receber os seguintes comandos:
			Encerrar todo o sistema (todas as aplicações são notificadas).
				Criar novos aeroportos.
				Suspender/ativar a aceitação de novos aviões por parte dos utilizadores.
				Listar todos os aeroportos, aviões e passageiros
		*/


}