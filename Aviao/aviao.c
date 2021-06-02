#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "../Control/estruturas.h"
#define NTHREADSPRINCIPAIS 2
#define NTHREADSMOVIMENTO 2
#define MUTEX_MOVIMENTO_AVIOES _T("Mutex para a movimentacao dos avioes")
#define MEMORIA_MOVIMENTO_AVIOES _T("Memoria para a movimentacao dos avioes")
#define MUTEX_INPUTS _T("Mutex dos Inputs")
#define MUTEX_FLAG_SAIR _T("Mutex da flag de sair")
//#define MUTEX_ENCERRAR_THREAD _T("Mutex para a flag de encerrar a thread")

//Estrutura de mutexes, semaforos, etc para passar entre funções
typedef struct {
	int id_processo_aviao;
	int x;
	int y;
} struct_posicoes_ocupadas;

typedef struct {
	struct_posicoes_ocupadas pos_ocup[10];
	int n_pos_ocup;
} struct_memoria_mapa;

typedef struct {
	HANDLE eventoInput;
	TCHAR com_total[TAM];
} struct_input;

typedef struct {
	HANDLE objMapGeral;
	HANDLE objMapParticular;
	HANDLE objMapMapa;
	HANDLE semafEscritos;
	HANDLE semafLidos;
	HANDLE mutexComunicacaoControl;
	HANDLE mutexComunicacoesAvioes;
	HANDLE mutexAviao;	
	HANDLE mutexMovimentoAvioes;
	HANDLE semafAvioesAtuais;
	HANDLE mutexInputs;
	HANDLE mutexFlagSair;
	struct_memoria_geral* ptrMemoriaGeral;
	struct_memoria_particular* ptrMemoriaParticular;
	struct_memoria_mapa* ptrMemoriaMapa;
	struct_aviao eu;
	int (*ptrmove)(int, int, int, int, int*, int*);
	HMODULE hDLL;
	struct_input input;
	BOOL flagSair;
} struct_util;



//Declaracao de Funcoes e Threads
DWORD WINAPI Principal(LPVOID param);
DWORD WINAPI VerificaEncerramentoControl(LPVOID param);
DWORD WINAPI Movimento(LPVOID param);
DWORD WINAPI OpcaoEncerrar(LPVOID param);
BOOL MenuInicial(struct_util* util);
BOOL MenuSecundario(struct_util* util);
BOOL Registo(struct_util* util, int capacidade, int velocidade, TCHAR aeroportoInicial[]);
void FechaHandles(struct_util* util);
BOOL InicializaUtil(struct_util* util);
BOOL ProximoDestino(struct_util* util,TCHAR destino[]);
void Encerra(struct_util* util);
BOOL verificaCoordenadasOcupadas(struct_util* util, int novo_x, int novo_y);
void AlteraPosicaoNaMemoria(struct_util* util, int novo_x, int novo_y);
void EnviarNovasCoordenadasAoControl(struct_util* util, int novo_x, int novo_y);
void AvisaControlDaChegada(struct_util* util);
void AlteraMinhasInformacoes(struct_util* util);

int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif
	HANDLE hthreadsPrincipais[NTHREADSPRINCIPAIS];
	struct_aeroporto origem;
	struct_aeroporto destino;
	struct_util util;
	util.eu.origem = &origem;
	util.eu.destino = &destino;
	//int indiceThread;

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
	WaitForSingleObject(util.mutexMovimentoAvioes, INFINITE);
	if (util.ptrMemoriaMapa->n_pos_ocup == NULL) {
		util.ptrMemoriaMapa->n_pos_ocup = 0;
	}
	ReleaseMutex(util.mutexMovimentoAvioes);
	//Registo do avião no Control
	if (Registo(&util, util.eu.lotacao, util.eu.velocidade, util.eu.origem->nome)==FALSE) {
		WaitForSingleObject(util.mutexComunicacoesAvioes, INFINITE);
		util.ptrMemoriaGeral->nrAvioes = --(util.ptrMemoriaGeral->nrAvioes);
		ReleaseMutex(util.mutexComunicacoesAvioes);
		FechaHandles(&util);
		return -1;
	}
	else {
		_tprintf(_T("Aviâo Registado com Sucesso!\n"));
	}

	//Criacao das Threads
	hthreadsPrincipais[0] = CreateThread(NULL, 0, Principal, &util, 0, NULL);
	if (hthreadsPrincipais[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread Principal!\n"));
		return -1;
	}
	hthreadsPrincipais[1] = CreateThread(NULL, 0, VerificaEncerramentoControl, &util, 0, NULL);
	if (hthreadsPrincipais[1] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread da Verificação do Encerramento do Control!\n"));
		return -1;
	}

	WaitForMultipleObjects(NTHREADSPRINCIPAIS, hthreadsPrincipais, FALSE, INFINITE);
	
	for (int i = 0; i < NTHREADSPRINCIPAIS; i++) {
		CloseHandle(hthreadsPrincipais[i]);
	}

	Encerra(&util);

	FechaHandles(&util);
	
}


//Codigo de Threads
DWORD WINAPI Principal(LPVOID param) {
	struct_util* util = (struct_util*)param;
	HANDLE hthreadsMovimento[NTHREADSMOVIMENTO];
	int indiceParou = -1;
	HANDLE threadEEvento[NTHREADSMOVIMENTO];


	util->input.eventoInput = CreateEvent(
		NULL,            //LPSECURITY_ATTRIBUTES lpEventAttributes,
		TRUE,            //BOOL bManualReset, reset MANUAL
		FALSE,            //BOOL bInitialState, FALSE = bloqueante/não sinalizado
		NULL            //LPCSTR lpName
	); 
	if (util->input.eventoInput == NULL) {
		_tprintf(TEXT("Erro ao criar o evento de input\n"));
		return -1;
	}

	hthreadsMovimento[0] = CreateThread(NULL, 0, OpcaoEncerrar, util, 0, NULL);
	if (hthreadsMovimento[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread da Comunicação!\n"));
		return -1;
	}

	do {
		util->flagSair = MenuInicial(util);
		if (util->flagSair == TRUE) {
			break;
		}
		util->flagSair = MenuSecundario(util);
		if (util->flagSair == TRUE) {
			break;
		}

		//Criacao das Threads
		hthreadsMovimento[1] = CreateThread(NULL, 0, Movimento, util, 0, NULL);
		if (hthreadsMovimento[1] == NULL) {
			_tprintf(_T("ERRO! Não foi possível criar a thread do menu!\n"));
			return -1;
		}

		threadEEvento[0] = util->input.eventoInput;
		threadEEvento[1] = hthreadsMovimento[1];

		indiceParou = WaitForMultipleObjects(NTHREADSMOVIMENTO,threadEEvento,FALSE,INFINITE);

		indiceParou = indiceParou - WAIT_OBJECT_0;

		if (indiceParou == 0) {
			WaitForSingleObject(util->mutexFlagSair, INFINITE);
			util->flagSair = TRUE;
			ReleaseMutex(util->mutexFlagSair);
			CloseHandle(hthreadsMovimento[1]);
			break;
		}

		CloseHandle(hthreadsMovimento[1]);

	} while (TRUE);

	CloseHandle(hthreadsMovimento[0]);

	return 0;
}

DWORD WINAPI Movimento(LPVOID param) {
	struct_util* util = (struct_util*)param;
	BOOL ocupada = FALSE;
	int novo_x;
	int novo_y;
	int resultado;
	double espera;
	espera = (1 / (double)util->eu.velocidade);
	espera = espera * 1000;

	do {	
		resultado = util->ptrmove(util->eu.pos_x, util->eu.pos_y, util->eu.destino->pos_x, util->eu.destino->pos_y, &novo_x, &novo_y);
		if (resultado == 0) {
			//se retornar 0 chegou ao destino e informa o control
			// -1 para quando estiver num aeroporto puderem estar mais que 1
			AlteraPosicaoNaMemoria(util, (-1), (-1));
			AvisaControlDaChegada(util);
			AlteraMinhasInformacoes(util);
			_tprintf(_T("Aterrei no aeroporto de %s"), util->eu.origem->nome);
			return 0;
		}
		if (resultado == 2) {
			return -1;
		}
		//verificar na memoria partilhada se alguem esta naquela posicao e se sim fazer algo
		do {
			ocupada = verificaCoordenadasOcupadas(util, novo_x, novo_y);
		} while (ocupada == TRUE);
		AlteraPosicaoNaMemoria(util, novo_x, novo_y);

		//enviar as novas coordenadas ao control
		EnviarNovasCoordenadasAoControl(util, novo_x, novo_y);

		//guardar em si as novas coordenadas
		util->eu.pos_x = novo_x;
		util->eu.pos_y = novo_y;
		_tprintf(_T("Avancei para as coordenadas x: %d ,y: %d\n"), util->eu.pos_x, util->eu.pos_y);

		WaitForSingleObject(util->mutexFlagSair, INFINITE);
		if (util->flagSair == TRUE) {
			_tprintf(_T("O Avião despenhou-se\n"));
			ReleaseMutex(util->mutexFlagSair);
			return 0;
		}
		ReleaseMutex(util->mutexFlagSair);
		Sleep(espera);

	} while (TRUE);
	return 0;
}

DWORD WINAPI OpcaoEncerrar(LPVOID param) {
	TCHAR com_total[TAM];
	com_total[0] = '\0';
	struct_util* util = (struct_util*)param;
	do {
		WaitForSingleObject(util->mutexInputs,INFINITE);
		_fgetts(com_total, TAM, stdin);
		com_total[_tcslen(com_total) - 1] = '\0';
		_tcscpy_s(util->input.com_total, _countof(util->input.com_total), com_total);
		SetEvent(util->input.eventoInput);
		ReleaseMutex(util->mutexInputs);
	} while ((_tcscmp(com_total, _T("encerrar")) != 0));
	
	return 0;
}



DWORD WINAPI VerificaEncerramentoControl(LPVOID param) {
	do {
		Sleep(10000);
	} while (TRUE);
	return 0;
}

//Codigo das funcoes


//Funções do movimento
BOOL verificaCoordenadasOcupadas(struct_util* util, int novo_x, int novo_y) {
	WaitForSingleObject(util->mutexMovimentoAvioes, INFINITE);
	for (int i = 0; i < util->ptrMemoriaMapa->n_pos_ocup; i++) {
		if (util->ptrMemoriaMapa->pos_ocup[i].x == novo_x && util->ptrMemoriaMapa->pos_ocup[i].y == novo_y) {
			return TRUE;
			break;
		}
	}
	ReleaseMutex(util->mutexMovimentoAvioes);
	return FALSE;
}

void AlteraPosicaoNaMemoria(struct_util* util, int novo_x, int novo_y) {
	WaitForSingleObject(util->mutexMovimentoAvioes, INFINITE);
	for (int i = 0; i < util->ptrMemoriaMapa->n_pos_ocup; i++) {
		if (util->ptrMemoriaMapa->pos_ocup[i].id_processo_aviao == util->eu.id_processo) {
			util->ptrMemoriaMapa->pos_ocup[i].x = novo_x;
			util->ptrMemoriaMapa->pos_ocup[i].y = novo_y;
			break;
		}
	}
	ReleaseMutex(util->mutexMovimentoAvioes);
}

void EnviarNovasCoordenadasAoControl(struct_util* util, int novo_x, int novo_y) {

	struct_aviao_com comunicacaoGeral;
	comunicacaoGeral.tipomsg = NOVAS_COORDENADAS;
	comunicacaoGeral.id_processo = util->eu.id_processo;
	comunicacaoGeral.pos_x = novo_x;
	comunicacaoGeral.pos_y = novo_y;

	WaitForSingleObject(util->semafLidos, INFINITE);
	WaitForSingleObject(util->mutexComunicacoesAvioes, INFINITE);

	CopyMemory(&util->ptrMemoriaGeral->coms_controlador[util->ptrMemoriaGeral->in], &comunicacaoGeral, sizeof(struct_aviao_com));
	util->ptrMemoriaGeral->in = (util->ptrMemoriaGeral->in + 1) % MAX_AVIOES;

	ReleaseMutex(util->mutexComunicacoesAvioes);
	ReleaseSemaphore(util->semafEscritos, 1, NULL);
}

void AvisaControlDaChegada(struct_util* util) {
	struct_aviao_com comunicacaoGeral;
	comunicacaoGeral.tipomsg = CHEGADA_AO_DESTINO;
	comunicacaoGeral.id_processo = util->eu.id_processo;

	WaitForSingleObject(util->semafLidos, INFINITE);
	WaitForSingleObject(util->mutexComunicacoesAvioes, INFINITE);

	CopyMemory(&util->ptrMemoriaGeral->coms_controlador[util->ptrMemoriaGeral->in], &comunicacaoGeral, sizeof(struct_aviao_com));
	util->ptrMemoriaGeral->in = (util->ptrMemoriaGeral->in + 1) % MAX_AVIOES;

	ReleaseMutex(util->mutexComunicacoesAvioes);
	ReleaseSemaphore(util->semafEscritos, 1, NULL);
}

void AlteraMinhasInformacoes(struct_util* util) {
	_tcscpy_s(util->eu.origem->nome, _countof(util->eu.origem->nome), util->eu.destino->nome);
	util->eu.origem->pos_x = util->eu.destino->pos_x;
	util->eu.origem->pos_y = util->eu.destino->pos_y;
	util->eu.pos_x = util->eu.origem->pos_x;
	util->eu.pos_y = util->eu.origem->pos_y;
	_tcscpy_s(util->eu.destino->nome, _countof(util->eu.destino->nome), _T(""));
	util->eu.destino->pos_x = -1;
	util->eu.destino->pos_y = -1;
}

//Funções de Menus
BOOL MenuInicial(struct_util* util) {

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
		_tprintf(_T("\n--------------------------------------------\n"));
		_tprintf(_T("\t\tMenu Principal\n"));
		_tprintf(_T("Localização atual: %s\n"), util->eu.origem);
		_tprintf(_T("\t-> Proximo Destino (destino <Aeroporto>)\n"));
		_tprintf(_T("\t-> Encerrar (encerrar)\n"));
		_tprintf(_T("Comando: "));
		WaitForSingleObject(util->mutexInputs, INFINITE);
		WaitForSingleObject(util->input.eventoInput,INFINITE);
		_tcscpy_s(com_total, _countof(com_total), util->input.com_total);
		ResetEvent(util->input.eventoInput);
		ReleaseMutex(util->mutexInputs);
		cont = _stscanf_s(com_total, _T("%s %s %s %s"), com, (unsigned)_countof(com), arg1, (unsigned)_countof(arg1), arg2, (unsigned)_countof(arg2), arg3, (unsigned)_countof(arg3));
		if (_tcscmp(com, _T("destino")) == 0 && cont == 2) {
			if (ProximoDestino(util, arg1) == FALSE) {
				continue;
			}
			return FALSE;
		}
		else {
			if (_tcscmp(com, _T("encerrar")) == 0 && cont == 1) {
				return TRUE;
			}
			else {
				_tprintf(_T("Comando Inválido!!!!\n"));
			}
		}

	} while (TRUE);
	return FALSE;
}

BOOL MenuSecundario(struct_util* util) {
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
		_tprintf(_T("\n--------------------------------------------\n"));
		_tprintf(_T("Proximo Destino -> %s\n"),util->eu.destino->nome);
		_tprintf(_T("\t-> Descolar (descolar)\n"));
		_tprintf(_T("\t-> Encerrar (encerrar)\n"));
		_tprintf(_T("Comando: "));

		WaitForSingleObject(util->mutexInputs, INFINITE);
		WaitForSingleObject(util->input.eventoInput, INFINITE);

		_tcscpy_s(com_total, _countof(com_total), util->input.com_total);

		ResetEvent(util->input.eventoInput);
		ReleaseMutex(util->mutexInputs);

		cont = _stscanf_s(com_total, _T("%s %s %s %s"), com, (unsigned)_countof(com), arg1, (unsigned)_countof(arg1), arg2, (unsigned)_countof(arg2), arg3, (unsigned)_countof(arg3));
		if (_tcscmp(com, _T("descolar")) == 0 && cont == 1) {
			return FALSE;
		}
		else {
			if (_tcscmp(com, _T("encerrar")) == 0 && cont == 1) {
				return TRUE;
			}
			else
			{
				_tprintf(_T("Comando Inválido!!!!\n"));
			}
		}

	} while (TRUE);
	return FALSE;
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

	if (comunicacaoParticular.tipomsg == AVIAO_RECUSADO) {
		_tprintf(_T("Erro! O avião foi recusado pelo Controlador!\n"));
		return FALSE;
	}
	util->eu.origem->pos_x = comunicacaoParticular.x_origem;
	util->eu.origem->pos_y = comunicacaoParticular.y_origem;
	util->eu.pos_x = comunicacaoParticular.x_origem;
	util->eu.pos_y = comunicacaoParticular.y_origem;

	return TRUE;
}

//Funções de preencher estruturas
BOOL InicializaUtil(struct_util* util) {
	TCHAR mem_aviao[TAM], mutex_aviao[TAM];
	_stprintf_s(mem_aviao, _countof(mem_aviao), MEMORIA_AVIAO, util->eu.id_processo);
	_stprintf_s(mutex_aviao, _countof(mutex_aviao), MUTEX_AVIAO, util->eu.id_processo);

	//util->encerraThread = FALSE;
	//DLL
	util->hDLL = LoadLibraryEx(_T("SO2_TP_DLL_2021.dll"), NULL, 0);
	if (util->hDLL != NULL) {

		FARPROC ptrGenerico; //Ponteiro genérico pode ser tanto para procurar a função dentro da DLL como uma variável
		ptrGenerico = GetProcAddress(util->hDLL, "move");
		if (ptrGenerico != NULL) {
			util->ptrmove = (int (*) (int, int, int, int, int*, int*)) ptrGenerico;
		}

	}

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

	//mutex para garantir que o control já inicializou a estrutura
	util->mutexMovimentoAvioes = CreateMutex(NULL, FALSE, MUTEX_MOVIMENTO_AVIOES);
	if (util->mutexMovimentoAvioes == NULL) {
		_tprintf(_T("Erro ao criar o mutex do movimento dos aviões!\n"));
		return FALSE;
	}

	//mutex para garantir que o control já inicializou a estrutura
	util->mutexInputs = CreateMutex(NULL, FALSE, MUTEX_INPUTS);
	if (util->mutexInputs == NULL) {
		_tprintf(_T("Erro ao criar o mutex do movimento dos aviões!\n"));
		return FALSE;
	}

	//mutex para a flag de sair
	util->mutexFlagSair = CreateMutex(NULL, FALSE, MUTEX_FLAG_SAIR);
	if (util->mutexFlagSair == NULL) {
		_tprintf(_T("Erro ao criar o mutex da flag de sair!\n"));
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

	//Memória Partilhada Movimentacao Avioes
	util->objMapMapa = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct_memoria_mapa), MEMORIA_MOVIMENTO_AVIOES);
	if (util->objMapMapa == NULL) {
		_tprintf(_T("Erro ao criar o File Mapping do Mapa!\n"));
		return FALSE;
	}

	util->ptrMemoriaMapa = (struct_memoria_mapa*)MapViewOfFile(util->objMapMapa, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (util->ptrMemoriaMapa == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoria do Mapa!\n"));
		return FALSE;
	}

	//Flag para encerrar o Aviao
	util->flagSair = FALSE;

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
	CloseHandle(util->mutexMovimentoAvioes);
	CloseHandle(util->objMapMapa);
	CloseHandle(util->mutexInputs);
	CloseHandle(util->input.eventoInput);
	FreeLibrary(util->hDLL);
	//CloseHandle(util->mutexEncerraThread);
	
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
