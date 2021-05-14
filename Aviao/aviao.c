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
BOOL Registo(struct_aviao* eu, int capacidade, int velocidade, TCHAR aeroportoInicial[]);


int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	struct_dados dados; //Podemos tirar penso
	HANDLE semafAvioesAtuais;
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
	if (Registo(&eu, eu.lotacao, eu.velocidade, eu.origem)==FALSE) {
		ReleaseSemaphore(semafAvioesAtuais, 1, NULL);
		CloseHandle(semafAvioesAtuais);
		return -1;
	}
	else {
		_tprintf(_T("Aviâo Registado com Sucesso!\n"));
	}
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

BOOL Registo(struct_aviao* eu, int capacidade, int velocidade, TCHAR aeroportoInicial[]) {
	struct_memoria_geral* ptrMemoriaGeral;
	struct_memoria_particular* ptrMemoriaParticular;
	HANDLE objMapGeral, objMapParticular;
	struct_aviao_com comunicacaoGeral;
	struct_controlador_com comunicacaoParticular;
	HANDLE semafEscritos, semafLidos, mutexComunicacaoControl, mutexComunicacoesAvioes, mutexAviao;
	TCHAR mem_aviao[TAM], mutex_aviao[TAM];
	_stprintf_s(mem_aviao, _countof(mem_aviao), MEMORIA_AVIAO, eu->id_processo);
	_stprintf_s(mutex_aviao, _countof(mutex_aviao), MUTEX_AVIAO, eu->id_processo);

	//mutex para os avioes escreverem um de cada vez
	mutexComunicacoesAvioes = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_AVIAO);
	if (mutexComunicacoesAvioes == NULL) {
		_tprintf(_T("Erro ao criar o mutex dos Avioes !\n"));
		return FALSE;
	}

	//mutex para a comunicacao control-aviao
	mutexAviao = CreateMutex(NULL, FALSE, mutex_aviao);
	if (mutexAviao == NULL) {
		_tprintf(_T("Erro ao criar o mutex do Aviao !\n"));
		return FALSE;
	}

	//mutex para garantir que o control já inicializou a estrutura
	mutexComunicacaoControl = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_CONTROL);
	if (mutexComunicacaoControl == NULL) {
		_tprintf(_T("Erro ao criar o mutex da primeira comunicacao!\n"));
		return FALSE;
	}

	//Criacao dos semaforos
	semafEscritos = CreateSemaphore(NULL, 0, MAX_AVIOES, SEMAFORO_AVIOES);
	if (semafEscritos == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos escritos!\n"));
		return FALSE;
	}

	semafLidos = CreateSemaphore(NULL, MAX_AVIOES, MAX_AVIOES, SEMAFORO_VAZIOS);
	if (semafLidos == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos lidos!\n"));
		return FALSE;
	}

	objMapGeral = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct_memoria_geral), MEMORIA_CONTROL);
	//Verificar se nao e NULL
	if (objMapGeral == NULL) {
		_tprintf(_T("Erro ao criar o File Mapping Geral!\n"));
		return FALSE;
	}
	ptrMemoriaGeral = (struct_memoria_geral*)MapViewOfFile(objMapGeral, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (ptrMemoriaGeral == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoria Geral!\n"));
		return FALSE;
	}

	objMapParticular = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct_memoria_particular), mem_aviao);
	//Verificar se nao e NULL
	if (objMapParticular == NULL) {
		_tprintf(_T("Erro ao criar o File Mapping Particular!\n"));
		return FALSE;
	}
	ptrMemoriaParticular = (struct_memoria_particular*)MapViewOfFile(objMapParticular, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (ptrMemoriaParticular == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoria Particular!\n"));
		return FALSE;
	}

	comunicacaoGeral.tipomsg = NOVO_AVIAO;
	comunicacaoGeral.id_processo = eu->id_processo;
	comunicacaoGeral.lotacao = eu->lotacao;
	comunicacaoGeral.velocidade = eu->velocidade;
	_tcscpy_s(comunicacaoGeral.nome_origem,_countof(comunicacaoGeral.nome_origem),eu->origem->nome);

	WaitForSingleObject(mutexComunicacaoControl, INFINITE);
	ReleaseMutex(mutexComunicacaoControl);

	WaitForSingleObject(semafLidos, INFINITE);
	WaitForSingleObject(mutexComunicacoesAvioes, INFINITE);

	ptrMemoriaGeral->nrAvioes = ++(ptrMemoriaGeral->nrAvioes);
	
	CopyMemory(&ptrMemoriaGeral->coms_controlador[ptrMemoriaGeral->in], &comunicacaoGeral, sizeof(struct_aviao_com));
	ptrMemoriaGeral->in = (ptrMemoriaGeral->in + 1) % MAX_AVIOES;
	
	ReleaseMutex(mutexComunicacoesAvioes);
	ReleaseSemaphore(semafEscritos,1,NULL);
	//Sleep(5000);
	//do {
		WaitForSingleObject(mutexAviao, INFINITE);
		CopyMemory(&comunicacaoParticular, &ptrMemoriaParticular->resposta[0], sizeof(struct_controlador_com));
		ReleaseMutex(mutexAviao);
	//} while (comunicacaoParticular.tipomsg == 0);
	_tprintf(_T("Tipo MSG: %d\n"),comunicacaoParticular.tipomsg);
	if (comunicacaoParticular.tipomsg == AVIAO_RECUSADO) {
		_tprintf(_T("Erro! O avião foi recusado pelo Controlador!\n"));
		return FALSE;
	}
	
	eu->origem->pos_x = comunicacaoParticular.x_origem;
	eu->origem->pos_y = comunicacaoParticular.y_origem;

	CloseHandle(objMapGeral);
	CloseHandle(semafEscritos);
	CloseHandle(semafLidos);
	CloseHandle(mutexComunicacaoControl);
	CloseHandle(mutexComunicacoesAvioes);
	CloseHandle(mutexAviao);
	return TRUE;
}

