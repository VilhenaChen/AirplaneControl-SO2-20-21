#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "../Control/estruturas.h"
#define NTHREADS 2

//Estrutura de mutexes, semaforos, etc para passar entre funções
typedef struct {
	HANDLE objMapGeral;
	HANDLE objMapParticular;
	HANDLE semafEscritos;
	HANDLE semafLidos;
	HANDLE mutexComunicacaoControl;
	HANDLE mutexComunicacoesAvioes;
	HANDLE mutexAviao;	
	HANDLE semafAvioesAtuais;
	struct_memoria_geral* ptrMemoriaGeral;
	struct_memoria_particular* ptrMemoriaParticular;
	struct_aviao eu;
} struct_util;

//Declaracao de Funcoes e Threads
DWORD WINAPI Movimento(LPVOID param);
DWORD WINAPI OpcaoEncerrar(LPVOID param);
void MenuInicial(struct_util* util);
void MenuSecundario(struct_util* util);
BOOL Registo(struct_util* util, int capacidade, int velocidade, TCHAR aeroportoInicial[]);
void FechaHandles(struct_util* util);
BOOL InicializaUtil(struct_util* util);
BOOL ProximoDestino(struct_util* util,TCHAR destino[]);
void Encerra(struct_util* util);

int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	HANDLE hthreads[NTHREADS];
	struct_aeroporto origem;
	struct_aeroporto destino;
	struct_util util;
	util.eu.origem = &origem;
	util.eu.destino = &destino;

	//Verificar se o Control já está a correr
	CreateMutex(0, FALSE, _T("Local\\$controlador$")); // try to create a named mutex
	if (GetLastError() != ERROR_ALREADY_EXISTS) // did the mutex already exist?
	{
		_tprintf(_T("ERRO! Nao existe nenhum controlador aberto!\n"));
		return -1; // quit; mutex is released automatically
	}

	//Verificar se os argumentos estão a ser passados de forma correta
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

	//Preencher informações do eu com o que foi passado
	util.eu.id_processo = GetCurrentProcessId();
	util.eu.lotacao = _tstoi(argv[1]);
	util.eu.velocidade = _tstoi(argv[2]);
	_tcscpy_s(util.eu.origem->nome,_countof(util.eu.origem->nome), argv[3]);

	//Semáforo para verificar se o avião se pode registar
	util.semafAvioesAtuais = CreateSemaphore(NULL, MAX_AVIOES, MAX_AVIOES, SEMAFORO_AVIOES_ATIVOS);
	if (util.semafAvioesAtuais == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos avioes atuais!\n"));
		return -1;
	}

	_tprintf(_T("A aguardar por um slot...\n"));
	WaitForSingleObject(util.semafAvioesAtuais, INFINITE);
	_tprintf(_T("Aviao Iniciado...\n"));

	InicializaUtil(&util);

	//Registo do avião no Control
	if (Registo(&util, util.eu.lotacao, util.eu.velocidade, util.eu.origem)==FALSE) {
		WaitForSingleObject(util.mutexComunicacoesAvioes, INFINITE);
		util.ptrMemoriaGeral->nrAvioes = --(util.ptrMemoriaGeral->nrAvioes);
		ReleaseMutex(util.mutexComunicacoesAvioes);
		FechaHandles(&util);
		return -1;
	}
	else {
		_tprintf(_T("Aviâo Registado com Sucesso!\n"));
	}

	MenuInicial(&util);
	MenuSecundario(&util);
	
	//Criacao das Threads
	hthreads[0] = CreateThread(NULL, 0, Movimento, &util, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread do menu!\n"));
		return -1;
	}
	hthreads[1] = CreateThread(NULL, 0, OpcaoEncerrar, &util, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread da Comunicação!\n"));
		return -1;
	}

	WaitForMultipleObjects(NTHREADS, hthreads, FALSE, INFINITE);

	//Fazer com que volte ao menu inicial se não pretender encerrar mesmo

	for (int i = 0; i < NTHREADS; i++) {
		CloseHandle(hthreads[i]);


	}

	FechaHandles(&util);
	
}


//Codigo de Threads

DWORD WINAPI Movimento(LPVOID param) {
	struct_util* util = (struct_util*)param;
	do {
		Sleep(10000);
	} while (TRUE);
	return 0;
}

DWORD WINAPI OpcaoEncerrar(LPVOID param) {
	TCHAR com_total[TAM];
	com_total[0] = '\0';
	struct_util* util = (struct_util*)param;
	do {
		_tprintf(_T("Encerrar (encerrar)\n"));
		_tprintf(_T("Comando: "));
		_fgetts(com_total, TAM, stdin);
		com_total[_tcslen(com_total) - 1] = '\0';
		if ((_tcscmp(com_total, _T("encerrar")) == 0)) {
			break;
		}
	} while (TRUE);

	Encerra(util);
	return 0;
}


//Codigo das funcoes

//Funções de Menus
void MenuInicial(struct_util* util) {

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
		_tprintf(_T("--------------------------------------------\n"));
		_tprintf(_T("Menu Principal\n"));
		_tprintf(_T("\t-> Proximo Destino (destino <Aeroporto>)\n"));
		_tprintf(_T("\t-> Encerrar (encerrar)\n"));
		_tprintf(_T("Comando: "));
		_fgetts(com_total, TAM, stdin);
		com_total[_tcslen(com_total) - 1] = '\0';
		cont = _stscanf_s(com_total, _T("%s %s %s %s"), com, (unsigned)_countof(com), arg1, (unsigned)_countof(arg1), arg2, (unsigned)_countof(arg2), arg3, (unsigned)_countof(arg3));
		if (_tcscmp(com, _T("destino")) == 0 && cont == 2) {
			if (ProximoDestino(util, arg1) == FALSE) {
				continue;
			}
			return;
		}
		else {
			if (_tcscmp(com, _T("encerrar")) == 0 && cont == 1) {
				Encerra(util);
				FechaHandles(util);
				exit(0);
			}
			else {
				_tprintf(_T("Comando Inválido!!!!\n"));
			}
		}

	} while (TRUE);
	return;
}

void MenuSecundario(struct_util* util) {
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
		_tprintf(_T("--------------------------------------------\n"));
		_tprintf(_T("Proximo Destino -> %s\n"),util->eu.destino->nome);
		_tprintf(_T("\t-> Descolar (descolar)\n"));
		_tprintf(_T("\t-> Encerrar (encerrar)\n"));
		_tprintf(_T("Comando: "));
		_fgetts(com_total, TAM, stdin);
		com_total[_tcslen(com_total) - 1] = '\0';
		cont = _stscanf_s(com_total, _T("%s %s %s %s"), com, (unsigned)_countof(com), arg1, (unsigned)_countof(arg1), arg2, (unsigned)_countof(arg2), arg3, (unsigned)_countof(arg3));
		if (_tcscmp(com, _T("descolar")) == 0 && cont == 1) {
			return;
		}
		else {
			if (_tcscmp(com, _T("encerrar")) == 0 && cont == 1) {
				Encerra(util);
				FechaHandles(util);
				exit(0);
			}
			else
			{
				_tprintf(_T("Comando Inválido!!!!\n"));
			}
		}

	} while (TRUE);
	return;
}

//Funções do Registo
BOOL Registo(struct_util* util, int capacidade, int velocidade, TCHAR aeroportoInicial[]) {

	struct_aviao_com comunicacaoGeral;
	struct_controlador_com comunicacaoParticular;

	comunicacaoGeral.tipomsg = NOVO_AVIAO;
	comunicacaoGeral.id_processo = util->eu.id_processo;
	comunicacaoGeral.lotacao = util->eu.lotacao;
	comunicacaoGeral.velocidade = util->eu.velocidade;
	_tcscpy_s(comunicacaoGeral.nome_origem,_countof(comunicacaoGeral.nome_origem), util->eu.origem->nome);

	WaitForSingleObject(util->mutexComunicacaoControl, INFINITE);
	ReleaseMutex(util->mutexComunicacaoControl);

	WaitForSingleObject(util->semafLidos, INFINITE);
	WaitForSingleObject(util->mutexComunicacoesAvioes, INFINITE);

	util->ptrMemoriaGeral->nrAvioes = ++(util->ptrMemoriaGeral->nrAvioes);
	
	CopyMemory(&util->ptrMemoriaGeral->coms_controlador[util->ptrMemoriaGeral->in], &comunicacaoGeral, sizeof(struct_aviao_com));
	util->ptrMemoriaGeral->in = (util->ptrMemoriaGeral->in + 1) % MAX_AVIOES;
	
	ReleaseMutex(util->mutexComunicacoesAvioes);
	ReleaseSemaphore(util->semafEscritos,1,NULL);
	//Sleep(5000);
	do {
		WaitForSingleObject(util->mutexAviao, INFINITE);
		CopyMemory(&comunicacaoParticular, &util->ptrMemoriaParticular->resposta[0], sizeof(struct_controlador_com));
		ReleaseMutex(util->mutexAviao);
	} while (comunicacaoParticular.tipomsg != AVIAO_CONFIRMADO && comunicacaoParticular.tipomsg != AVIAO_RECUSADO );
	_tprintf(_T("Tipo MSG: %d\n"),comunicacaoParticular.tipomsg);

	if (comunicacaoParticular.tipomsg == AVIAO_RECUSADO) {
		_tprintf(_T("Erro! O avião foi recusado pelo Controlador!\n"));
		return FALSE;
	}
	util->eu.origem->pos_x = comunicacaoParticular.x_origem;
	util->eu.origem->pos_y = comunicacaoParticular.y_origem;

	return TRUE;
}

//Funções de preencher estruturas
BOOL InicializaUtil(struct_util* util) {
	TCHAR mem_aviao[TAM], mutex_aviao[TAM];
	_stprintf_s(mem_aviao, _countof(mem_aviao), MEMORIA_AVIAO, util->eu.id_processo);
	_stprintf_s(mutex_aviao, _countof(mutex_aviao), MUTEX_AVIAO, util->eu.id_processo);

	//mutex para os avioes escreverem um de cada vez
	util->mutexComunicacoesAvioes = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_AVIAO);
	if (util->mutexComunicacoesAvioes == NULL) {
		_tprintf(_T("Erro ao criar o mutex dos Avioes !\n"));
		return FALSE;
	}

	//mutex para a comunicacao control-aviao
	util->mutexAviao = CreateMutex(NULL, FALSE, mutex_aviao);
	if (util->mutexAviao == NULL) {
		_tprintf(_T("Erro ao criar o mutex do Aviao !\n"));
		return FALSE;
	}

	//mutex para garantir que o control já inicializou a estrutura
	util->mutexComunicacaoControl = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_CONTROL);
	if (util->mutexComunicacaoControl == NULL) {
		_tprintf(_T("Erro ao criar o mutex da primeira comunicacao!\n"));
		return FALSE;
	}

	//Criacao dos semaforos

	//Semáforo para controlar as comunicações escritas
	util->semafEscritos = CreateSemaphore(NULL, 0, MAX_AVIOES, SEMAFORO_AVIOES);
	if (util->semafEscritos == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos escritos!\n"));
		return FALSE;
	}

	//Semáforo para controlar as comunicações lidas
	util->semafLidos = CreateSemaphore(NULL, MAX_AVIOES, MAX_AVIOES, SEMAFORO_VAZIOS);
	if (util->semafLidos == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos lidos!\n"));
		return FALSE;
	}

	//Memória Partilhada

	//Memória Partilhada Aviao-Control
	util->objMapGeral = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct_memoria_geral), MEMORIA_CONTROL);
	if (util->objMapGeral == NULL) {
		_tprintf(_T("Erro ao criar o File Mapping Geral!\n"));
		return FALSE;
	}

	util->ptrMemoriaGeral = (struct_memoria_geral*)MapViewOfFile(util->objMapGeral, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (util->ptrMemoriaGeral == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoria Geral!\n"));
		return FALSE;
	}

	//Memória Partilhada Control-Aviao
	util->objMapParticular = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct_memoria_particular), mem_aviao);
	if (util->objMapParticular == NULL) {
		_tprintf(_T("Erro ao criar o File Mapping Particular!\n"));
		return FALSE;
	}

	util->ptrMemoriaParticular = (struct_memoria_particular*)MapViewOfFile(util->objMapParticular, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (util->ptrMemoriaParticular == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoria Particular!\n"));
		return FALSE;
	}

	return TRUE;
}

//Funcao para fechar os handles
void FechaHandles(struct_util* util) {
	ReleaseSemaphore(util->semafAvioesAtuais, 1, NULL);
	CloseHandle(util->semafAvioesAtuais);
	CloseHandle(util->objMapGeral);
	CloseHandle(util->semafEscritos);
	CloseHandle(util->semafLidos);
	CloseHandle(util->mutexComunicacaoControl);
	CloseHandle(util->mutexComunicacoesAvioes);
	CloseHandle(util->mutexAviao);
	
}

//Funcoes Destino
BOOL ProximoDestino(struct_util* util, TCHAR destino[]) {

	struct_aviao_com comunicacaoGeral;
	struct_controlador_com comunicacaoParticular;

	comunicacaoGeral.tipomsg = NOVO_DESTINO;
	comunicacaoGeral.id_processo = util->eu.id_processo;
	_tcscpy_s(comunicacaoGeral.nome_destino, _countof(comunicacaoGeral.nome_destino), destino);

	WaitForSingleObject(util->semafLidos, INFINITE);
	WaitForSingleObject(util->mutexComunicacoesAvioes, INFINITE);

	CopyMemory(&util->ptrMemoriaGeral->coms_controlador[util->ptrMemoriaGeral->in], &comunicacaoGeral, sizeof(struct_aviao_com));
	util->ptrMemoriaGeral->in = (util->ptrMemoriaGeral->in + 1) % MAX_AVIOES;

	ReleaseMutex(util->mutexComunicacoesAvioes);
	ReleaseSemaphore(util->semafEscritos, 1, NULL);

	do {
		WaitForSingleObject(util->mutexAviao, INFINITE);
		CopyMemory(&comunicacaoParticular, &util->ptrMemoriaParticular->resposta[0], sizeof(struct_controlador_com));
		ReleaseMutex(util->mutexAviao);
	} while (comunicacaoParticular.tipomsg != DESTINO_VERIFICADO && comunicacaoParticular.tipomsg != DESTINO_REJEITADO);
	_tprintf(_T("Tipo MSG: %d\n"), comunicacaoParticular.tipomsg);

	if (comunicacaoParticular.tipomsg == DESTINO_REJEITADO) {
		_tprintf(_T("Erro! O destino inserido não é válido!\n"));
		return FALSE;
	}

	_tcscpy_s(util->eu.destino->nome, _countof(util->eu.destino->nome), destino);
	util->eu.destino->pos_x = comunicacaoParticular.x_destino;
	util->eu.destino->pos_y = comunicacaoParticular.y_destino;
	return TRUE;
}
//Funções Encerrar
void Encerra(struct_util* util) {

	struct_aviao_com comunicacaoGeral;
	comunicacaoGeral.tipomsg = ENCERRAR_AVIAO;
	comunicacaoGeral.id_processo = util->eu.id_processo;

	WaitForSingleObject(util->semafLidos, INFINITE);
	WaitForSingleObject(util->mutexComunicacoesAvioes, INFINITE);

	util->ptrMemoriaGeral->nrAvioes = --(util->ptrMemoriaGeral->nrAvioes);

	CopyMemory(&util->ptrMemoriaGeral->coms_controlador[util->ptrMemoriaGeral->in], &comunicacaoGeral, sizeof(struct_aviao_com));
	util->ptrMemoriaGeral->in = (util->ptrMemoriaGeral->in + 1) % MAX_AVIOES;

	ReleaseMutex(util->mutexComunicacoesAvioes);
	ReleaseSemaphore(util->semafEscritos, 1, NULL);
}
