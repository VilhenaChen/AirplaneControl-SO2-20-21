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
void Registo(struct_aviao* eu, int capacidade, int velocidade, TCHAR aeroportoInicial[]);


int _tmain(int argc, TCHAR* argv[]) {
	struct_dados dados; //Podemos tirar penso
	HANDLE semafAvioesAtuais;
	int capacidade = _tstoi(argv[1]);
	int velocidade = _tstoi(argv[2]);
	TCHAR aeroportoInicial = argv[3];
	struct_aviao eu;
	struct_aeroporto origem;
	struct_aeroporto destino;
	eu.origem = &origem;
	eu.destino = &destino;
	CreateMutex(0, FALSE, _T("Local\\$controlador$")); // try to create a named mutex
	if (GetLastError() != ERROR_ALREADY_EXISTS) // did the mutex already exist?
	{
		_tprintf(_T("ERRO! Nao existe nenhum controlador aberto!\n"));
		return -1; // quit; mutex is released automatically
	}

	if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
		_tprintf(_T("ERRO! Não foram passados os argumentos necessários ao lançamento do Aviao!\n"));
		return -1; // quit; mutex is released automatically
	}

	if (argv[1] <= 0 ) {
		_tprintf(_T("ERRO! A capacidade do Avião tem de ser superior a 0!\n"));
		return -1; // quit; mutex is released automatically
	}
	if (argv[2] <= 0) {
		_tprintf(_T("ERRO! A velocidade do Avião tem de ser superior a 0!\n"));
		return -1; // quit; mutex is released automatically
	}

	eu.id_processo = GetCurrentProcessId();
	eu.lotacao = _tstoi(argv[1]);
	eu.velocidade = _tstoi(argv[2]);
	_tcscpy_s(eu.origem->nome,_countof(eu.origem->nome), argv[3]);

	semafAvioesAtuais = CreateSemaphore(NULL, MAX_AVIOES, MAX_AVIOES, SEMAFORO_AVIOES_ATIVOS);
	if (semafAvioesAtuais == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos avioes atuais!\n"));
		return -1;
	}


	_tprintf(_T("A aguardar por um slot...\n"));
	WaitForSingleObject(semafAvioesAtuais, INFINITE);
	_tprintf(_T("Aviao Iniciado...\n"));
	fflush(stdin);
	Registo(&eu, capacidade, velocidade, aeroportoInicial);
	//Receber o resultado do registo e caso nao tenha sido aceite terminar fazendo todos os releases e etc.
	MenuInicial(&dados);
	ReleaseSemaphore(semafAvioesAtuais, 1, NULL);
	CloseHandle(semafAvioesAtuais);
	
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

void Registo(struct_aviao* eu, int capacidade, int velocidade, TCHAR aeroportoInicial[]) {
	struct_memoria_geral* ptrMemoria;
	HANDLE objMap;
	struct_aviao_com comunicacao;
	HANDLE semafEscritos, semafLidos, mutexComunicacaoControl, mutexComunicacoesAvioes, mutexAviao;

	//mutex para os avioes escreverem um de cada vez
	mutexComunicacoesAvioes = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_AVIAO);
	if (mutexComunicacoesAvioes == NULL) {
		_tprintf(_T("Erro ao criar o mutex dos Avioes !\n"));
		exit(-1);
	}

	//mutex para a comunicacao control-aviao
	mutexAviao = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_AVIAO);
	if (mutexAviao == NULL) {
		_tprintf(_T("Erro ao criar o mutex do Aviao !\n"));
		exit(-1);
	}

	//mutex para garantir que o control já inicializou a estrutura
	mutexComunicacaoControl = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_CONTROL);
	if (mutexComunicacaoControl == NULL) {
		_tprintf(_T("Erro ao criar o mutex da primeira comunicacao!\n"));
		exit(-1);
	}

	//Criacao dos semaforos
	semafEscritos = CreateSemaphore(NULL, 0, MAX_AVIOES, SEMAFORO_AVIOES);
	if (semafEscritos == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos escritos!\n"));
		exit(-1);
	}

	semafLidos = CreateSemaphore(NULL, MAX_AVIOES, MAX_AVIOES, SEMAFORO_VAZIOS);
	if (semafLidos == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos lidos!\n"));
		exit(-1);
	}
	
	objMap= CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct_memoria_geral), MEMORIA_CONTROL);
	//Verificar se nao e NULL
	if (objMap == NULL) {
		_tprintf(_T("Erro ao criar o File Mapping Geral!\n"));
		exit(-1);
	}
	ptrMemoria = (struct_memoria_geral*)MapViewOfFile(objMap, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (ptrMemoria == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoria Geral!\n"));
		exit(-1);
	}

	comunicacao.tipomsg = NOVO_AVIAO;
	comunicacao.id_processo = eu->id_processo;
	comunicacao.lotacao = eu->lotacao;
	comunicacao.velocidade = eu->velocidade;
	_tcscpy_s(comunicacao.nome_origem,_countof(comunicacao.nome_origem),eu->origem->nome);

	WaitForSingleObject(mutexComunicacaoControl, INFINITE);
	ReleaseMutex(mutexComunicacaoControl);

	WaitForSingleObject(semafLidos, INFINITE);
	WaitForSingleObject(mutexComunicacoesAvioes, INFINITE);

	ptrMemoria->nrAvioes = ++(ptrMemoria->nrAvioes);
	
	CopyMemory(&ptrMemoria->coms_controlador[ptrMemoria->in], &comunicacao, sizeof(struct_aviao_com));
	ptrMemoria->in = (ptrMemoria->in + 1) % MAX_AVIOES;
	
	ReleaseMutex(mutexComunicacoesAvioes);
	ReleaseSemaphore(semafEscritos,1,NULL);

	CloseHandle(objMap);
	CloseHandle(semafEscritos);
	CloseHandle(semafLidos);
	CloseHandle(mutexComunicacaoControl);
	CloseHandle(mutexComunicacoesAvioes);
	CloseHandle(mutexAviao);
}

