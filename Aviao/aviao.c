#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "../Control/estruturas.h"
#define NTHREADS 2

//Declaracao de Funcoes e Threads
DWORD WINAPI Viagem(LPVOID param);
DWORD WINAPI Movimento(LPVOID param);
void MenuInicial(struct_dados* dados);
void registo(struct_dados* dados);


int _tmain(int argc, TCHAR* argv[]) {
	struct_dados dados;
	HANDLE semafAvioesAtuais;
	

	CreateMutex(0, FALSE, "Local\\$controlador$"); // try to create a named mutex
	if (GetLastError() != ERROR_ALREADY_EXISTS) // did the mutex already exist?
	{
		_tprintf(_T("ERRO! Nao existe nenhum controlador aberto!\n"));
		return -1; // quit; mutex is released automatically
	}


	semafAvioesAtuais = CreateSemaphore(NULL, MAX_AVIOES, MAX_AVIOES, SEMAFORO_AVIOES_ATIVOS);
	if (semafAvioesAtuais == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos avioes atuais!\n"));
		return -1;
	}
	_tprintf(_T("A aguardar por um slot...\n"));
	WaitForSingleObject(semafAvioesAtuais, INFINITE);
	_tprintf(_T("Aviao Iniciado...\n"));
	fflush(stdin);

	MenuInicial(&dados);
	ReleaseSemaphore(semafAvioesAtuais, 1, NULL);
	
}


//Codigo de Threads

DWORD WINAPI Viagem(LPVOID param) {
	return 0;
}

DWORD WINAPI Movimento(LPVOID param) {
	return 0;
}

//Codigo das funcoes

void MenuInicial(struct_dados* dados) {

	TCHAR com_total[TAM];
	TCHAR com[TAM], arg1[TAM], arg2[TAM], arg3[TAM];

	int cont;
	do {
		com_total[0] = '\0';
		com[0] = '\0';
		arg1[0] = '\0';
		arg2[0] = '\0';
		arg3[0] = '\0';
		cont = 0;
		_tprintf(_T("Menu Principal\n"));
		_tprintf(_T("\t-> Proximo Destino (destino <Aeroporto>)\n"));
		_tprintf(_T("\t-> Iniciar Viagem (descolar)\n"));
		_tprintf(_T("\t-> Encerrar (encerrar)\n"));
		_tprintf(_T("Comando: "));
		_fgetts(com_total, TAM, stdin);
		com_total[_tcslen(com_total) - 1] = '\0';
		cont = _stscanf_s(com_total, _T("%s %s %s %s"), com, (unsigned)_countof(com), arg1, (unsigned)_countof(arg1), arg2, (unsigned)_countof(arg2), arg3, (unsigned)_countof(arg3));
		if (_tcscmp(com, _T("destino")) == 0 && cont == 2) {

		}
		else {
			if (_tcscmp(com, _T("descolar")) == 0 && cont == 1) {

			}
			else
			{
				if (_tcscmp(com, _T("encerrar")) == 0 && cont == 1) {

				}
				else {
					_tprintf(_T("Comando Inválido!!!!\n"));
					fflush(stdin);
				}
			}
		}

	} while (TRUE);
	return 0;
}

void registo(struct_dados* dados) {
	struct_memoria_geral* ptrMemoria;
	HANDLE objMap;
	struct_aviao_com comunicacao;
	HANDLE semafEscritos, semafLidos, semafAvioesAtuais, mutexComunicacaoControl, mutexComunicacoesAvioes, mutexAviao;

	//mutex para os avioes escreverem um de cada vez
	mutexComunicacoesAvioes = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_AVIAO);
	if (mutexComunicacoesAvioes == NULL) {
		_tprintf(_T("Erro ao criar o mutex dos Avioes !\n"));
		return -1;
	}

	//mutex para a comunicacao control-aviao
	mutexAviao = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_AVIAO);
	if (mutexAviao == NULL) {
		_tprintf(_T("Erro ao criar o mutex do Aviao !\n"));
		return -1;
	}

	//mutex para garantir que o control já inicializou a estrutura
	mutexComunicacaoControl = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_CONTROL);
	if (mutexComunicacaoControl == NULL) {
		_tprintf(_T("Erro ao criar o mutex da primeira comunicacao!\n"));
		return -1;
	}

	//Criacao dos semaforos
	semafEscritos = CreateSemaphore(NULL, 0, MAX_AVIOES, SEMAFORO_AVIOES);
	if (semafEscritos == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos escritos!\n"));
		return -1;
	}

	semafLidos = CreateSemaphore(NULL, MAX_AVIOES, MAX_AVIOES, SEMAFORO_VAZIOS);
	if (semafLidos == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos lidos!\n"));
		return -1;
	}
	
	objMap= CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct_memoria_geral), MEMORIA_CONTROL);
	//Verificar se nao e NULL
	if (objMap == NULL) {
		_tprintf(_T("Erro ao criar o File Mapping Geral!\n"));
		return -1;
	}
	ptrMemoria = (struct_memoria_geral*)MapViewOfFile(objMap, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (ptrMemoria == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoria Geral!\n"));
		return -1;
	}

	WaitForSingleObject(mutexComunicacaoControl, INFINITE);
	WaitForSingleObject(mutexComunicacoesAvioes, INFINITE);

	ptrMemoria->nrAvioes = ++(ptrMemoria->nrAvioes);

	ReleaseMutex(mutexComunicacaoControl);
	ReleaseMutex(mutexComunicacoesAvioes);

}
