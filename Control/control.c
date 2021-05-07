#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "estruturas.h"
#define NTHREADS 1


//Declaracao de Funcoes e Threads
DWORD WINAPI Menu(LPVOID param);


int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif
	
	HANDLE hthreads[NTHREADS];
	struct_dados dados;
	HKEY chave; //Hanle para a chave depois de criada/aberta
	DWORD resultado_chave; //inteiro long, indica o que aconteceu à chave, se foi criada ou aberta ou não
	TCHAR nome_chave[TAM] = TEXT("SOFTWARE\\TEMP\\TP_SO2"), nome_par_avioes[TAM] = TEXT("maxAvioes"),nome_par_aeroportos[TAM] = TEXT("maxAeroportos");
	DWORD par_valor_avioes = 10, par_valor_aeroportos = 10;

	CreateMutex(0, FALSE, "Local\\$controlador$"); // try to create a named mutex
	if (GetLastError() == ERROR_ALREADY_EXISTS) // did the mutex already exist?
	{
		_tprintf(_T("ERRO! Já existe uma instância deste programa a correr!\n"));
		return -1; // quit; mutex is released automatically
	}
	_tprintf(_T("--------------------- CONTROLADOR ---------------------\n"));

	if (RegCreateKeyEx(HKEY_CURRENT_USER,
		nome_chave /*caminho*/,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE /*opção default, funcionaria igual com 0*/,
		KEY_ALL_ACCESS /*acesso total a todas as funcionalidades da chave*/,
		NULL /*quem pode ter acesso, neste caso apenas o proprio*/,
		&chave,
		&resultado_chave)
		!= ERROR_SUCCESS) /*diferente de sucesso*/
	{
		_tprintf(TEXT("ERRO! A chave não foi criada nem aberta!\n"));
		return -1;
	}
	if (resultado_chave == REG_CREATED_NEW_KEY) {
		_tprintf(TEXT("A chave %s foi criada!\n"), nome_chave);
	}
	else {
		if (resultado_chave == REG_OPENED_EXISTING_KEY) {
			_tprintf(TEXT("A chave %s foi aberta!\n"), nome_chave);
		}
	}

	if (RegSetValueEx(
		chave,		/*handle chave aberta*/
		nome_par_avioes	/*nome parametro*/,
		0,
		REG_DWORD,
		&par_valor_avioes, // para não dar warning
		sizeof(par_valor_avioes)  //tamanho do que está escrito na string incluindo o /0
	)!=ERROR_SUCCESS)
	{
		_tprintf(TEXT("ERRO ao aceder ao atributo %s !\n"),nome_par_avioes);
		return -1;
	}

	if (RegSetValueEx(
		chave,		/*handle chave aberta*/
		nome_par_aeroportos	/*nome parametro*/,
		0,
		REG_DWORD,
		&par_valor_aeroportos, // para não dar warning
		sizeof(par_valor_aeroportos)  //tamanho do que está escrito na string incluindo o /0
	)!=ERROR_SUCCESS)
	{
		_tprintf(TEXT("ERRO ao aceder ao atributo %s !\n"),nome_par_aeroportos);
		return -1;
	}

	hthreads[0] = CreateThread(NULL, 0, Menu, &dados, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread do menu!\n"));
		return -1; 
	}

	WaitForMultipleObjects(NTHREADS, hthreads, FALSE, INFINITE);

	for (int i = 0; i < NTHREADS; i++) {
		CloseHandle(hthreads[i]);
	}

	RegCloseKey(chave);

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

	return 0;
}