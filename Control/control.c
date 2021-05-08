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
//DWORD WINAPI Comunicacao(LPVOID param);
BOOL CriaAeroporto(TCHAR nome[TAM], int x, int y, struct_dados* dados);
void Lista(struct_dados* dados);


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

	
	dados.n_aeroportos_atuais = 0;
	
	CreateMutex(0, FALSE, "Local\\$controlador$"); // try to create a named mutex
	if (GetLastError() == ERROR_ALREADY_EXISTS) // did the mutex already exist?
	{
		_tprintf(_T("ERRO! Já existe uma instância deste programa a correr!\n"));
		return -1; // quit; mutex is released automatically
	}
	

	//Criacao da Key onde serao guardados o max de avioes e Aeroportos
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

	//Criacao dos semaforos
	/*semafAvioes = CreateSemaphore(NULL, 0, TAM, SEMAFORO_AVIOES);
	if (semafAvioes == NULL) {
		_tprintf(_T("Erro ao criar o semáforo Avioes!\n"));
		return -1;
	}
	semafVazios = CreateSemaphore(NULL, TAM, TAM, SEMAFORO_VAZIOS);
	if (semafVazios == NULL) {
		_tprintf(_T("Erro ao criar o semáforo Vazios!\n"));
		return -1;
	}
	*/
	/*mutexControl = CreateMutex(NULL, FALSE, MUTEX_CONTROLADOR);
	if (mutexControl == NULL) {
		_tprintf(_T("Erro ao criar o mutex do produtor!\n"));
		return -1;
	}*/

	//Criacao das Threads
	hthreads[0] = CreateThread(NULL, 0, Menu, &dados, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread do menu!\n"));
		return -1; 
	}
	/*hthreads[1] = CreateThread(NULL, 0, Comunicacao, &dados, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread da Comunicação!\n"));
		return -1; 
	}*/

	WaitForMultipleObjects(NTHREADS, hthreads, FALSE, INFINITE);

	for (int i = 0; i < NTHREADS; i++) {
		CloseHandle(hthreads[i]);
	}

	RegCloseKey(chave);

	return 0;
}


//Codigo de Threads
DWORD WINAPI Menu(LPVOID param) {
	struct_dados* dados = (struct_dados*)param;
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
			CriaAeroporto(arg1,_tstoi(arg2),_tstoi(arg3),dados);
		}
		else {
			if (_tcscmp(com, _T("suspender")) == 0 && cont == 2) {

			}
			else {
				if (_tcscmp(com, _T("lista")) == 0) {
					Lista(dados);
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


/*DWORD WINAPI Comunicacao(LPVOID param) {
	int id_aviao;
	struct_dados* dados = (struct_dados*) param;
	struct_memoria_particular* ptrMemoriaParticular;
	HANDLE objMapParticular;
	struct_aviao_com comunicacao_particular;
	//Lê da memoria partilhada geral

	//Responde a cada aviao atraves da memoria partilhada particular
	objMapParticular = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,sizeof(struct_memoria_particular),(MEMORIA_AVIAO,id_aviao));
	//Verificar se nao e NULL
	if (objMapParticular == NULL) {
		_tprintf(_T("Erro ao criar o File Mapping Particular!\n"));
		return -1;
	}

	ptrMemoriaParticular = (struct_memoria_particular*)MapViewOfFile(objMapParticular, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (ptrMemoriaParticular == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoriaParicular!\n"));
		return -1;
	}

	// colocar em comunicacao particular o que é pretendido enviar
	WaitForSingleObject(&dados->avioes[id_aviao].mutex, INFINITE);
	CopyMemory(&ptrMemoriaParticular->aviao,&comunicacao_particular,sizeof(struct_aviao_com));
	ReleaseMutex(&dados->avioes[id_aviao].mutex);
}*/

//Codigo de Funcoes

BOOL CriaAeroporto(TCHAR nome[TAM], int x, int y, struct_dados* dados) {
	if (dados->n_aeroportos_atuais < MAX_AEROPORTOS) {
		if (x > TAM_MAP || x <= 0 || y > TAM_MAP || y <= 0) {
			_tprintf(_T("Erro! O valor de X e Y deve ser entre 1 e 1000!!!\n"));
			return FALSE;
		}

		for (int i = 0; i < dados->n_aeroportos_atuais; i++) {
			if (_tcscmp(dados->aeroportos[i].nome, nome) == 0) {
				_tprintf(_T("Erro! Os aeroportos devem ter nomes distintos!!!\n"));
				return FALSE;
			}
			if ((dados->aeroportos[i].pos_x <= x+10 && dados->aeroportos[i].pos_x >= x-10) && (dados->aeroportos[i].pos_y <= y+10 && dados->aeroportos[i].pos_y >= y-10)) {
				_tprintf(_T("Erro! Os aeroportos devem estar a uma distância de 10 posições!!!\n"));
				return FALSE;
			}
		}
		
		_tcscpy_s(dados->aeroportos[dados->n_aeroportos_atuais].nome, _countof(dados->aeroportos[dados->n_aeroportos_atuais].nome), nome);
		dados->aeroportos[dados->n_aeroportos_atuais].pos_x = x;
		dados->aeroportos[dados->n_aeroportos_atuais].pos_y = y;
		dados->n_aeroportos_atuais++;
		return TRUE;
	}
	_tprintf(_T("Erro! Já foi atingido o máximo de aeroportos\n"));
	return FALSE;
}

void Lista(struct_dados* dados) {
	_tprintf(_T("Lista de Aeroportos\n"));
	for (int i = 0; i < dados->n_aeroportos_atuais; i++) {
		_tprintf(_T("%s\n"),dados->aeroportos[i].nome);
		_tprintf(_T("\tCoordenada X: %d\n"),dados->aeroportos[i].pos_x);
		_tprintf(_T("\tCoordenada Y: %d\n"),dados->aeroportos[i].pos_y);
	}

}