#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "estruturas.h"
#define NTHREADS 4
#define MUTEX_ACEDER_AVIOES _T("Mutex para aceder a estrutura dos avioes")
#define MUTEX_ACEDER_AEROPORTOS _T("Mutex para aceder a estrutura dos aeroportos")
#define MUTEX_ACEDER_PASSAGEIROS _T("Mutex para aceder a estrutura dos passageiros")
#define MUTEX_RESPONDER_PASSAGEIROS _T("Mutex para esponder aos passageiros")


//Estrutura onde sao guardados os dados do control
typedef struct {
	struct_aeroporto aeroportos[MAX_AEROPORTOS];
	struct_aviao avioes[MAX_AVIOES];
	struct_passageiro passageiros[MAX_PASSAGEIROS];
	int n_aeroportos_atuais;
	int n_avioes_atuais;
	int n_passageiros_atuais;
	HANDLE mutex_comunicacao;
	HANDLE mutex_acede_avioes;
	HANDLE mutex_acede_aeroportos;
	HANDLE mutex_acede_passageiros;
	HANDLE semafAvioesAtuais;
	BOOL suspenso;
	HANDLE mutex_resposta_passageiro;
	HANDLE hPipePassageiro;
	HANDLE hMeuPipe;
} struct_dados;

//Declaracao de Funcoes e Threads
DWORD WINAPI Menu(LPVOID param);
DWORD WINAPI ComunicacaoAviao(LPVOID param);
DWORD WINAPI ComunicacaoPassageiro(LPVOID param);
DWORD WINAPI HeartbeatAvioes(LPVOID param);
void InicializaDados(struct_dados* dados);
void CriaLeChave(struct_dados* dados);
BOOL RespondeAoAviao(struct_dados* dados, struct_aviao_com* comunicacaoGeral);
void preencheInformacoesSemResposta(struct_dados* dados, struct_aviao_com* comunicacaoGeral);
BOOL CriaAeroporto(TCHAR nome[TAM], int x, int y, struct_dados* dados);
void InsereAviao(struct_dados* dados, int idProcesso, int capacidade, int velocidade, int indiceAeroporto);
void EliminaAviao(struct_dados* dados, int idProcesso);
void EliminaPassageiro(struct_dados* dados, int idProcesso);
void InserePassageiro(struct_dados* dados, TCHAR origem[TAM], TCHAR destino[TAM], TCHAR nome[TAM], int tempoEspera, int idProcesso);
void PreencheResposta(struct_dados* dados,struct_msg_control_passageiro* msgControl, int tipoResposta, int idProcesso);
BOOL VerificaPassageiroAceite(struct_dados* dados, struct_msg_passageiro_control* msg);
BOOL RespondeAoPassageiro(struct_dados* dados, struct_msg_control_passageiro* msg, int idProcesso);
void Lista(struct_dados* dados);
int getIndiceAviao(int id_processo, struct_dados* dados);
void preencheComunicacaoParticularEAtualizaInformacoes(struct_dados* dados, struct_aviao_com* comunicacaoGeral, struct_controlador_com* comunicacaoParticular);
int getIndiceAeroporto(struct_dados* dados, TCHAR aeroporto[]);
int getIndicePassageiro(struct_dados* dados, int idProcesso);
void suspendeAvioes(struct_dados* dados);
void retomaAvioes(struct_dados* dados);
BOOL EmbarcaPassageirosSePossivel(struct_dados* dados);
struct_aviao* getAviaoComOrigemDestino(struct_dados* dados, struct_aeroporto* origem, struct_aeroporto* destino);
BOOL verificaAviaoEmVoo(struct_aviao* aviao);
void InformaNovasCoordenadasPassageiro(struct_dados* dados, struct_aviao* aviao);
void InformaAviaoDespenhado(struct_dados* dados, struct_aviao* aviao);
void encerrar(struct_dados* dados);


int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif
	
	HANDLE hthreads[NTHREADS];
	struct_dados dados;
	
	CreateMutex(0, FALSE, _T("Local\\$controlador$")); // Tenta criar um named Mutex
	if (GetLastError() == ERROR_ALREADY_EXISTS) // O Mutex ja existe?
	{
		_tprintf(_T("ERRO! Já existe uma instância deste programa a correr!\n"));
		return -1; // Caso ja exista o programa termina garantindo que apenas existe uma instancia do mesmo
	}
	InicializaDados(&dados);
	CriaLeChave(&dados);

	//Criacao das Threads
	hthreads[0] = CreateThread(NULL, 0, Menu, &dados, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread do menu!\n"));
		return -1; 
	}
	hthreads[1] = CreateThread(NULL, 0, ComunicacaoAviao, &dados, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread da Comunicação do Avião!\n"));
		return -1; 
	}
	hthreads[2] = CreateThread(NULL, 0, ComunicacaoPassageiro, &dados, 0, NULL);
	if (hthreads[2] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread da Comunicação do Passageiro!\n"));
		return -1;
	}
	hthreads[3] = CreateThread(NULL, 0, HeartbeatAvioes, &dados, 0, NULL);
	if (hthreads[3] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread do HeartBeat dos avioes!\n"));
		return -1;
	}
	WaitForMultipleObjects(NTHREADS, hthreads, FALSE, INFINITE);

	for (int i = 0; i < NTHREADS; i++) {
		CloseHandle(hthreads[i]);
	}

	CloseHandle(dados.mutex_comunicacao);
	CloseHandle(dados.mutex_acede_aeroportos);
	CloseHandle(dados.mutex_acede_avioes);
	CloseHandle(dados.semafAvioesAtuais);
	CloseHandle(dados.mutex_acede_passageiros);
	CloseHandle(dados.mutex_resposta_passageiro);
	return 0;
}


//Codigo de Threads

//Thread Menu
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
			WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
			CriaAeroporto(arg1,_tstoi(arg2),_tstoi(arg3),dados);
			ReleaseMutex(dados->mutex_acede_aeroportos);
		}
		else {
			if (_tcscmp(com, _T("suspender")) == 0 && cont == 2) {
				if (_tcscmp(arg1, _T("on")) == 0) {
					if (dados->suspenso == FALSE) {
						dados->suspenso = TRUE;
						WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
						suspendeAvioes(dados);
						ReleaseMutex(dados->n_avioes_atuais);
						_tprintf(_T("Suspensão de aviões ligada\n"));
					}
					else {
						_tprintf(_T("O registo já se encontra suspenso\n"));
					}
				}
				else if(_tcscmp(arg1, _T("off")) == 0) {
					if (dados->suspenso == TRUE) {
						dados->suspenso = FALSE;
						WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
						retomaAvioes(dados);
						ReleaseMutex(dados->n_avioes_atuais);
	
						_tprintf(_T("Suspensão de aviões desligada\n"));
					}
					else {
						_tprintf(_T("O registo não se encontra suspenso\n"));
					}
				}
			}
			else {
				if (_tcscmp(com, _T("lista")) == 0) {
					Lista(dados);
				}
				else {
					if (_tcscmp(com, _T("encerrar")) == 0) {
						encerrar(dados);
						return 0;
					}
					else {
						_tprintf(_T("Comando Inválido!!!!\n"));
						fflush(stdin);
					}
				}
			}
		}

	} while (TRUE);
	return 0;
}

//Thread Comunicação
DWORD WINAPI ComunicacaoAviao(LPVOID param) {
	int id_aviao = 0;
	struct_dados* dados = (struct_dados*) param;
	struct_memoria_geral* ptrMemoriaGeral;
	HANDLE objMapGeral;
	struct_aviao_com comunicacaoGeral;
	HANDLE semafEscritos, semafLidos;

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
	
	objMapGeral = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct_memoria_geral), MEMORIA_CONTROL);
	if (objMapGeral == NULL) {
		_tprintf(_T("Erro ao criar o File Mapping Geral!\n"));
		return -1;
	}
	ptrMemoriaGeral = (struct_memoria_geral*)MapViewOfFile(objMapGeral, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (ptrMemoriaGeral == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoria Geral!\n"));
		return -1;
	}

	ptrMemoriaGeral->in = 0;
	ptrMemoriaGeral->out = 0;
	ptrMemoriaGeral->nrAvioes = 0;

	ReleaseMutex(dados->mutex_comunicacao);
	while (TRUE) {
		WaitForSingleObject(semafEscritos, INFINITE);
		CopyMemory(&comunicacaoGeral, &ptrMemoriaGeral->coms_controlador[ptrMemoriaGeral->out], sizeof(struct_aviao_com));
		ptrMemoriaGeral->out = (ptrMemoriaGeral->out + 1) % MAX_AVIOES;
		ReleaseSemaphore(semafLidos, 1, NULL);
		if (comunicacaoGeral.tipomsg == NOVO_AVIAO) {
			if (RespondeAoAviao(dados, &comunicacaoGeral) == FALSE) {
				break;
			}
		}
		else {
			if (getIndiceAviao(comunicacaoGeral.id_processo, dados) != -1) {
				if (comunicacaoGeral.tipomsg == NOVO_DESTINO) {
					if (RespondeAoAviao(dados, &comunicacaoGeral) == FALSE) {
						break;
					}
				}
				else {
					preencheInformacoesSemResposta(dados, &comunicacaoGeral);
				}
			}
		}
	}
	CloseHandle(objMapGeral);
	CloseHandle(semafEscritos);
	CloseHandle(semafLidos);
	
}

DWORD WINAPI ComunicacaoPassageiro(LPVOID param) {
	struct_dados* dados = (struct_dados*)param;
	//TCHAR buffer[256];
	BOOL retorno;
	DWORD n;
	struct_msg_passageiro_control mensagemLida;
	struct_msg_control_passageiro mensagemEscrita;
	int tipo_resposta;

	dados->hMeuPipe = CreateNamedPipe(PIPE_CONTROL_GERAL, PIPE_ACCESS_INBOUND, PIPE_WAIT |
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, 1,
		sizeof(struct_msg_passageiro_control), sizeof(struct_msg_passageiro_control), 1000, NULL);
	if (dados->hMeuPipe == INVALID_HANDLE_VALUE) {
		//_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)"));
		_tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError());
		exit(-1);
	}

	while (1) {
		_tprintf(TEXT(" Esperar ligação de um passageiro... (ConnectNamedPipe)\n"));
		if (!ConnectNamedPipe(dados->hMeuPipe, NULL)) {
			_tprintf(TEXT("[ERRO] Ligação ao leitor! (ConnectNamedPipe\n"));
			exit(-1);
		}
		retorno = ReadFile(dados->hMeuPipe, &mensagemLida, sizeof(struct_msg_passageiro_control), &n, NULL);
		if (!retorno || !n) {
			_tprintf(TEXT("[Control] %d %d... (ReadFile)\n"), retorno, n);
			break;
		}
		_tprintf(TEXT("[Control] Recebi %d bytes: '%d %s %s %s %d'... (ReadFile)\n"), n, mensagemLida.id_processo, mensagemLida.nome, mensagemLida.origem, mensagemLida.destino, mensagemLida.tempo_espera);
		switch(mensagemLida.tipo){
			case NOVO_PASSAGEIRO:
				if (VerificaPassageiroAceite(dados, &mensagemLida)) {
					_tprintf(_T("Foi Aceite!"));
					InserePassageiro(dados, mensagemLida.origem, mensagemLida.destino, mensagemLida.nome, mensagemLida.tempo_espera, mensagemLida.id_processo);
					EmbarcaPassageirosSePossivel(dados);
					_tprintf(_T("Foi Inserido!"));
					tipo_resposta = PASSAGEIRO_ACEITE;
					WaitForSingleObject(dados->mutex_resposta_passageiro,INFINITE);
					PreencheResposta(dados, &mensagemEscrita, tipo_resposta, mensagemLida.id_processo);
					_tprintf(_T("Foi Preenchida a resposta!"));
					RespondeAoPassageiro(dados, &mensagemEscrita, mensagemLida.id_processo);
					ReleaseMutex(dados->mutex_resposta_passageiro);
				}
				else {
					tipo_resposta = PASSAGEIRO_RECUSADO;
					WaitForSingleObject(dados->mutex_resposta_passageiro, INFINITE);
					PreencheResposta(dados, &mensagemEscrita, tipo_resposta, mensagemLida.id_processo);
					_tprintf(_T("Foi Preenchida a resposta!"));
					RespondeAoPassageiro(dados, &mensagemEscrita, mensagemLida.id_processo);					
					ReleaseMutex(dados->mutex_resposta_passageiro);
				}
				break;
			case ENCERRAR_PASSAGEIRO:
				_tprintf(_T("O passageiro vai encerrar!"));
				EliminaPassageiro(dados, mensagemLida.id_processo);
				_tprintf(_T("Eliminei o Passageiro!"));
				break;
		}
		
		_tprintf(_T("Foi Respondido ao Passageiro!"));
		_tprintf(TEXT("[ESCRITOR] Desligar o pipe (DisconnectNamedPipe)\n"));
		if (!DisconnectNamedPipe(dados->hMeuPipe)) {
			_tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
			exit(-1);
		}
	}

	return 0;
}

DWORD WINAPI HeartbeatAvioes(LPVOID param) {
	struct_dados* dados = (struct_dados*)param;	
	LARGE_INTEGER tempo;
	HANDLE waitable_timer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (NULL == waitable_timer) {
		_tprintf(_T("Erro ao criar o waitable timer!\n"));
		return 1;
	}

	do {
		tempo.QuadPart = -(10000000LL);

		if (!SetWaitableTimer(waitable_timer, &tempo, 0, NULL, NULL, 0))
		{
			_tprintf(_T("Erro ao iniciar o waitable timer!\n"));
			return 0;
		}
		WaitForSingleObject(waitable_timer, INFINITE);

		for (int i = 0; i < dados->n_avioes_atuais; i++) {
			dados->avioes[i].tempo_inatividade++;
			if (dados->avioes[i].tempo_inatividade == 3) {
				_tprintf(_T("O avião %d (PID: %d) ficou inativo!\n"), i, dados->avioes[i].id_processo);
				WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
				EliminaAviao(dados, dados->avioes[i].id_processo);
				ReleaseMutex(dados->mutex_acede_avioes);
				InformaAviaoDespenhado(dados, &dados->avioes[i]);
			}
		}

	} while (1);
	return 0;
}

//Codigo de Funcoes

//Inicializa Dados
void InicializaDados(struct_dados* dados) {

	dados->n_aeroportos_atuais = 0;
	dados->n_avioes_atuais = 0;
	dados->n_passageiros_atuais = 0;
	dados->suspenso = FALSE;

	dados->mutex_resposta_passageiro = CreateMutex(NULL, FALSE, MUTEX_RESPONDER_PASSAGEIROS);
	if (dados->mutex_resposta_passageiro == NULL) {
		_tprintf(_T("Erro ao criar o mutex do meu pipe!\n"));
		return -1;
	}

	dados->mutex_comunicacao = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_CONTROL);
	if (dados->mutex_comunicacao == NULL) {
		_tprintf(_T("Erro ao criar o mutex da primeira comunicacao!\n"));
		return -1;
	}

	dados->mutex_acede_avioes = CreateMutex(NULL, FALSE, MUTEX_ACEDER_AVIOES);
	if (dados->mutex_acede_avioes == NULL) {
		_tprintf(_T("Erro ao criar o mutex de aceder aos avioes!\n"));
		return -1;
	}

	dados->mutex_acede_aeroportos = CreateMutex(NULL, FALSE, MUTEX_ACEDER_AEROPORTOS);
	if (dados->mutex_acede_aeroportos == NULL) {
		_tprintf(_T("Erro ao criar o mutex de aceder aos aeroportos!\n"));
		return -1;
	}

	dados->mutex_acede_passageiros = CreateMutex(NULL, FALSE, MUTEX_ACEDER_PASSAGEIROS);
	if (dados->mutex_acede_passageiros == NULL) {
		_tprintf(_T("Erro ao criar o mutex de aceder aos aeroportos!\n"));
		return -1;
	}

	dados->semafAvioesAtuais = CreateSemaphore(NULL, MAX_AVIOES, MAX_AVIOES, SEMAFORO_AVIOES_ATIVOS);
	if (dados->semafAvioesAtuais == NULL) {
		_tprintf(_T("Erro ao criar o semáforo dos avioes atuais!\n"));
		return -1;
	}

	dados->mutex_acede_passageiros = CreateMutex(NULL, FALSE, MUTEX_PASSAGEIROS);
	if (dados->mutex_acede_passageiros == NULL) {
		_tprintf(_T("Erro ao criar o mutex de aceder aos passageiros!\n"));
		return -1;
	}
}

//Funcao de criar ou ler a chave com os numeros maximos de Aeroportos e Avioes
void CriaLeChave(struct_dados* dados) {
	HKEY chave; //Handle para a chave depois de criada/aberta
	DWORD resultado_chave; //inteiro long, indica o que aconteceu à chave, se foi criada ou aberta ou não
	TCHAR nome_chave[TAM] = _T("SOFTWARE\\TEMP\\TP_SO2"), nome_par_avioes[TAM] = _T("maxAvioes"),nome_par_aeroportos[TAM] = _T("maxAeroportos");
	DWORD par_valor_avioes = MAX_AVIOES, par_valor_aeroportos = MAX_AEROPORTOS;

	//Tentativa de leitura da chave


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
		exit(-1);
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
		exit(-1);
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
		exit(-1);
	}
	RegCloseKey(chave);
}


//Funções de Comunicação
BOOL RespondeAoAviao(struct_dados* dados, struct_aviao_com* comunicacaoGeral) { //Responde a cada aviao atraves da memoria partilhada particular
	HANDLE objMapParticular, mutexParticular;
	struct_memoria_particular* ptrMemoriaParticular;
	struct_controlador_com comunicacaoParticular;
	TCHAR mem_aviao[TAM], mutex_aviao[TAM];
	_stprintf_s(mem_aviao, _countof(mem_aviao),MEMORIA_AVIAO, comunicacaoGeral->id_processo);
	_stprintf_s(mutex_aviao, _countof(mutex_aviao), MUTEX_AVIAO, comunicacaoGeral->id_processo);
	fflush(stdout);

	objMapParticular = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct_memoria_particular), mem_aviao);
	fflush(stdout);
	if (objMapParticular == NULL) {
		_tprintf(_T("Erro ao criar o File Mapping Particular!\n"));
		return FALSE;
	}
	fflush(stdout);
	ptrMemoriaParticular = (struct_memoria_particular*)MapViewOfFile(objMapParticular, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (ptrMemoriaParticular == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoria Paricular!\n"));
		return FALSE;
	}
	fflush(stdout);
	mutexParticular = CreateMutex(NULL, FALSE, mutex_aviao);
	if (mutexParticular == NULL) {
		_tprintf(_T("Erro ao criar o mutex do aviao!\n"));
		return FALSE;
	}
	WaitForSingleObject(mutexParticular, INFINITE);
	preencheComunicacaoParticularEAtualizaInformacoes(dados, comunicacaoGeral, &comunicacaoParticular);
	CopyMemory(&ptrMemoriaParticular->resposta[0], &comunicacaoParticular, sizeof(struct_controlador_com));
	ReleaseMutex(mutexParticular);
	return TRUE;
}

void preencheComunicacaoParticularEAtualizaInformacoes(struct_dados* dados, struct_aviao_com* comunicacaoGeral, struct_controlador_com* comunicacaoParticular) {
	int indiceAeroporto;
	int indiceAviao;
	switch (comunicacaoGeral->tipomsg)
	{
	case NOVO_AVIAO:

		WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
		indiceAeroporto = getIndiceAeroporto(dados, comunicacaoGeral->nome_origem);
		ReleaseMutex(dados->mutex_acede_aeroportos);
		if (indiceAeroporto == -1) {
			comunicacaoParticular->tipomsg = AVIAO_RECUSADO;
			return;
		}
		else {
			comunicacaoParticular->tipomsg = AVIAO_CONFIRMADO;

			WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
			comunicacaoParticular->x_origem = dados->aeroportos[indiceAeroporto].pos_x;
			comunicacaoParticular->y_origem = dados->aeroportos[indiceAeroporto].pos_y;
			ReleaseMutex(dados->mutex_acede_aeroportos);


			InsereAviao(dados, comunicacaoGeral->id_processo, comunicacaoGeral->lotacao, comunicacaoGeral->velocidade, indiceAeroporto);

			return;
		}
		break;
	case NOVO_DESTINO:

		WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
		indiceAeroporto = getIndiceAeroporto(dados, comunicacaoGeral->nome_destino);
		ReleaseMutex(dados->mutex_acede_aeroportos);

		if (indiceAeroporto == -1) {
			comunicacaoParticular->tipomsg = DESTINO_REJEITADO;
			return;
		}
		else {
			comunicacaoParticular->tipomsg = DESTINO_VERIFICADO;

			WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
			comunicacaoParticular->x_destino = dados->aeroportos[indiceAeroporto].pos_x;
			comunicacaoParticular->y_destino = dados->aeroportos[indiceAeroporto].pos_y;
			ReleaseMutex(dados->mutex_acede_aeroportos);

			WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
			indiceAviao = getIndiceAviao(comunicacaoGeral->id_processo, dados);

			WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
			dados->avioes[indiceAviao].destino = &dados->aeroportos[indiceAeroporto];
			EmbarcaPassageirosSePossivel(dados);
			ReleaseMutex(dados->mutex_acede_aeroportos);

			ReleaseMutex(dados->mutex_acede_avioes);
			return;
		}
		break;
	}
}

void preencheInformacoesSemResposta(struct_dados* dados, struct_aviao_com* comunicacaoGeral) {
	int indiceAviao = -1;
	struct_msg_control_passageiro msg;
	if (comunicacaoGeral->tipomsg != ENCERRAR_AVIAO) {
		WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
		indiceAviao = getIndiceAviao(comunicacaoGeral->id_processo, dados);
		dados->avioes[indiceAviao].tempo_inatividade = 0;
		ReleaseMutex(dados->mutex_acede_avioes);
	}

	switch (comunicacaoGeral->tipomsg)
	{
		case NOVAS_COORDENADAS:
			WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
			indiceAviao = getIndiceAviao(comunicacaoGeral->id_processo, dados);
			dados->avioes[indiceAviao].pos_x = comunicacaoGeral->pos_x;
			dados->avioes[indiceAviao].pos_y = comunicacaoGeral->pos_y;
			InformaNovasCoordenadasPassageiro(dados, &dados->avioes[indiceAviao]);
			_tprintf(_T("Avião: %d - %d --- Posição: %d, %d\n"), indiceAviao, dados->avioes[indiceAviao].id_processo, dados->avioes[indiceAviao].pos_x, dados->avioes[indiceAviao].pos_y);
			ReleaseMutex(dados->mutex_acede_avioes);
			break;
		case CHEGADA_AO_DESTINO:
			WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
			indiceAviao = getIndiceAviao(comunicacaoGeral->id_processo, dados);
			dados->avioes[indiceAviao].pos_x = dados->avioes[indiceAviao].destino->pos_x;
			dados->avioes[indiceAviao].pos_y = dados->avioes[indiceAviao].destino->pos_y;
			dados->avioes[indiceAviao].origem = dados->avioes[indiceAviao].destino;
			dados->avioes[indiceAviao].destino = NULL;
			dados->avioes[indiceAviao].nr_passageiros_atuais = 0;
			_tprintf(_T("Avião: %d - %d --- Aterrou no Aeroporto de %s\n"), indiceAviao, dados->avioes[indiceAviao].id_processo, dados->avioes[indiceAviao].origem->nome);
			ReleaseMutex(dados->mutex_acede_avioes);
			break;
		case ENCERRAR_AVIAO:
			WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
			indiceAviao = getIndiceAviao(comunicacaoGeral->id_processo, dados);
			_tprintf(_T("O Aviao %d (PID: %d) encerrou!"), indiceAviao, dados->avioes[indiceAviao].id_processo);
			InformaAviaoDespenhado(dados, &dados->avioes[indiceAviao]);
			EliminaAviao(dados, comunicacaoGeral->id_processo);
			ReleaseMutex(dados->mutex_acede_avioes);
			break;
	}
}

//Funções dos Aeroportos
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
		dados->aeroportos[dados->n_aeroportos_atuais].passageiros_atuais = 0;
		dados->n_aeroportos_atuais++;
		return TRUE;
	}
	_tprintf(_T("Erro! Já foi atingido o máximo de aeroportos\n"));
	return FALSE;
}
int getIndiceAeroporto(struct_dados* dados,  TCHAR aeroporto[]) {
	for (int i = 0; i < dados->n_aeroportos_atuais; i++) {
		if (_tcscmp(dados->aeroportos[i].nome, aeroporto) == 0) {
			return i;
		}
	}
	return -1;
}

int getIndicePassageiro(struct_dados* dados, int idProcesso) {
	for (int i = 0; i < dados->n_passageiros_atuais; i++) {
		if (dados->passageiros[i].id_processo==idProcesso) {
			return i;
		}
	}
	return -1;
}



//Funções dos Avioes
int getIndiceAviao(int id_processo, struct_dados* dados) {
	for (int i = 0; i < dados->n_avioes_atuais; i++) {
		if (dados->avioes[i].id_processo == id_processo) {
			return i;
		}
	}
	return -1;
}

void InsereAviao(struct_dados* dados, int idProcesso, int capacidade, int velocidade, int indiceAeroporto) {
	WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
	dados->avioes[dados->n_avioes_atuais].id_processo = idProcesso;
	dados->avioes[dados->n_avioes_atuais].lotacao = capacidade;
	dados->avioes[dados->n_avioes_atuais].velocidade = velocidade;
	dados->avioes[dados->n_avioes_atuais].nr_passageiros_atuais = 0;
	dados->avioes[dados->n_avioes_atuais].tempo_inatividade = 0;

	WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
	dados->avioes[dados->n_avioes_atuais].pos_x = dados->aeroportos[indiceAeroporto].pos_x;
	dados->avioes[dados->n_avioes_atuais].pos_y = dados->aeroportos[indiceAeroporto].pos_y;
	dados->avioes[dados->n_avioes_atuais].origem = &dados->aeroportos[indiceAeroporto];
	ReleaseMutex(dados->mutex_acede_aeroportos);

	dados->avioes[dados->n_avioes_atuais].destino = NULL;
	dados->n_avioes_atuais = dados->n_avioes_atuais++;
	ReleaseMutex(dados->mutex_acede_avioes);
}

void EliminaAviao(struct_dados* dados, int idProcesso) {
	struct_aviao avioesAux[MAX_AVIOES];
	int indiceApagar=-1;
	int n_avioes = dados->n_avioes_atuais;
	for (int i = 0; i < n_avioes; i++) {
		if (dados->avioes[i].id_processo == idProcesso) {
			indiceApagar = i;
			break;
		}
	}
	int j = indiceApagar;
	j++;
	for (int i = indiceApagar; i < n_avioes; i++,j++)
	{
		if (j != (n_avioes)) {
			dados->avioes[i].id_processo = dados->avioes[j].id_processo;
			dados->avioes[i].lotacao = dados->avioes[j].lotacao;
			dados->avioes[i].origem = dados->avioes[j].origem;
			dados->avioes[i].pos_x = dados->avioes[j].pos_x;
			dados->avioes[i].pos_y = dados->avioes[j].pos_y;
			dados->avioes[i].velocidade = dados->avioes[j].velocidade;
			dados->avioes[i].destino = dados->avioes[j].destino;
			dados->avioes[i].nr_passageiros_atuais = dados->avioes[j].nr_passageiros_atuais;
		}
		else {
			dados->avioes[i].id_processo = 0;
			dados->avioes[i].lotacao = 0;
			dados->avioes[i].origem = NULL;
			dados->avioes[i].pos_x = 0;
			dados->avioes[i].pos_y = 0;
			dados->avioes[i].velocidade = 0;
			dados->avioes[i].destino = NULL;
			dados->avioes[i].nr_passageiros_atuais = 0;
		}
	}
	n_avioes--;
	dados->n_avioes_atuais = n_avioes;
}

void EliminaPassageiro(struct_dados* dados, int idProcesso) {
	struct_aviao passageirosAux[MAX_PASSAGEIROS];
	int indiceApagar=-1;
	WaitForSingleObject(dados->mutex_acede_passageiros, INFINITE);
	int n_passageiros = dados->n_passageiros_atuais;
	for (int i = 0; i < n_passageiros; i++) {
		if (dados->passageiros[i].id_processo == idProcesso) {
			indiceApagar = i;
			break;
		}
	}
	int j = indiceApagar;
	j++;
	for (int i = indiceApagar; i < dados->n_passageiros_atuais; i++,j++)
	{
		if (j != (n_passageiros)) {
			dados->passageiros[i].id_processo = dados->passageiros[j].id_processo;
			_tcscpy_s(dados->passageiros[i].nome, _countof(dados->passageiros[i].nome), dados->passageiros[j].nome);
			dados->passageiros[i].tempo_espera = dados->passageiros[j].tempo_espera;
			dados->passageiros[i].origem = dados->passageiros[j].origem;
			dados->passageiros[i].destino = dados->passageiros[j].destino;
			dados->passageiros[i].aviao = dados->passageiros[j].aviao;
		}
		else {
			dados->passageiros[i].id_processo = 0;
			_tcscpy_s(dados->passageiros[i].nome, _countof(dados->passageiros[i].nome), "");
			dados->passageiros[i].tempo_espera = 0;
			dados->passageiros[i].origem = NULL;
			dados->passageiros[i].destino = NULL;
			dados->passageiros[i].aviao = NULL;
		}
	}
	n_passageiros--;
	dados->n_passageiros_atuais = n_passageiros;
	ReleaseMutex(dados->mutex_acede_passageiros);
}

//Funcoes dos Passageiros
void InserePassageiro(struct_dados* dados, TCHAR origem[TAM], TCHAR destino[TAM], TCHAR nome[TAM], int tempoEspera, int idProcesso) {
	WaitForSingleObject(dados->mutex_acede_passageiros, INFINITE);
	_tcscpy_s(dados->passageiros[dados->n_passageiros_atuais].nome, _countof(dados->passageiros[dados->n_passageiros_atuais].nome), nome);
	dados->passageiros[dados->n_passageiros_atuais].aviao = NULL;
	dados->passageiros[dados->n_passageiros_atuais].id_processo = idProcesso;
	WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
	dados->passageiros[dados->n_passageiros_atuais].origem = &dados->aeroportos[getIndiceAeroporto(dados, origem)];
	dados->passageiros[dados->n_passageiros_atuais].destino = &dados->aeroportos[getIndiceAeroporto(dados, destino)];
	ReleaseMutex(dados->mutex_acede_aeroportos);
	dados->n_passageiros_atuais++;
	ReleaseMutex(dados->mutex_acede_passageiros);
}

void PreencheResposta(struct_dados* dados, struct_msg_control_passageiro* msgControl, int tipoResposta, int idProcesso) {	
	msgControl->tipo = tipoResposta;
	switch (tipoResposta) {
		case PASSAGEIRO_RECUSADO:
			break;
		case PASSAGEIRO_ACEITE:
			WaitForSingleObject(dados->mutex_acede_passageiros, INFINITE);
			msgControl->x_origem=dados->passageiros[getIndicePassageiro(dados,idProcesso)].origem->pos_x;
			msgControl->y_origem=dados->passageiros[getIndicePassageiro(dados,idProcesso)].origem->pos_y;
			msgControl->x_destino=dados->passageiros[getIndicePassageiro(dados,idProcesso)].destino->pos_x;
			msgControl->y_destino=dados->passageiros[getIndicePassageiro(dados,idProcesso)].destino->pos_y;
			ReleaseMutex(dados->mutex_acede_passageiros);
			break;
		case NOVA_COORDENADA:
			WaitForSingleObject(dados->mutex_acede_passageiros, INFINITE);
			msgControl->x_atual = dados->passageiros[getIndicePassageiro(dados, idProcesso)].aviao->pos_x;
			msgControl->y_atual = dados->passageiros[getIndicePassageiro(dados, idProcesso)].aviao->pos_y;
			ReleaseMutex(dados->mutex_acede_passageiros);
			break;
		case AVIAO_DESPENHOU:
			break;
		case AVIAO_ATRIBUIDO:
			WaitForSingleObject(dados->mutex_acede_passageiros, INFINITE);
			msgControl->id_processo = dados->passageiros[getIndicePassageiro(dados, idProcesso)].aviao->id_processo;
			ReleaseMutex(dados->mutex_acede_passageiros);
			break;
	}
}

BOOL VerificaPassageiroAceite(struct_dados* dados, struct_msg_passageiro_control* msg) {
	WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
	if (getIndiceAeroporto(dados,msg->origem) == -1 || getIndiceAeroporto(dados, msg->destino) == -1) {
		ReleaseMutex(dados->mutex_acede_aeroportos);
		return FALSE;
	}
	ReleaseMutex(dados->mutex_acede_aeroportos);
	return TRUE;
}

BOOL RespondeAoPassageiro(struct_dados* dados, struct_msg_control_passageiro* msg, int idProcesso) {
	DWORD n;
	TCHAR nomePipe[TAM];
	HANDLE pipePassag;
	_stprintf_s(nomePipe, _countof(nomePipe), PIPE_PASSAG_PARTICULAR, idProcesso);

	//ligação named pipe control e escrita para o mesmo
	_tprintf(TEXT("[LEITOR] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), PIPE_PASSAG_PARTICULAR);
	if (!WaitNamedPipe(nomePipe, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_PASSAG_PARTICULAR);
		exit(-1);
	}

	_tprintf(TEXT("[LEITOR] Ligação ao pipe do control... (CreateFile)\n"));
	pipePassag = CreateFile(nomePipe, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (pipePassag == NULL) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_PASSAG_PARTICULAR);
		exit(-1);
	}
	_tprintf(TEXT("[LEITOR] Liguei-me...\n"));

	if (!WriteFile(pipePassag, msg, sizeof(struct_msg_control_passageiro), &n, NULL)) {
		_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
		exit(-1);
	}

	CloseHandle(pipePassag);

}


//Funções lista
void Lista(struct_dados* dados) {
	WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
	WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
	WaitForSingleObject(dados->mutex_acede_passageiros, INFINITE);
	_tprintf(_T("Lista de Aeroportos\n"));
	for (int i = 0; i < dados->n_aeroportos_atuais; i++) {
		_tprintf(_T("%s\n"),dados->aeroportos[i].nome);
		_tprintf(_T("\tCoordenada X: %d\n"),dados->aeroportos[i].pos_x);
		_tprintf(_T("\tCoordenada Y: %d\n"),dados->aeroportos[i].pos_y);
	}
	_tprintf(_T("Lista de Avioes\n"));
	for (int i = 0; i < dados->n_avioes_atuais; i++) {
		_tprintf(_T("Aviao: %d - %d\n"), i, dados->avioes[i].id_processo);
		_tprintf(_T("\tCapacidade: %d\n"), dados->avioes[i].lotacao);
		_tprintf(_T("\tOrigem: %s\n"), dados->avioes[i].origem->nome);
		if (dados->avioes[i].destino != NULL) {
			_tprintf(_T("\tDestino: %s\n"), dados->avioes[i].destino->nome);
		}
		_tprintf(_T("\tPosição Atual: x: %d\t y: %d\n"), dados->avioes[i].pos_x, dados->avioes[i].pos_y);
		_tprintf(_T("\tVelocidade: %d\n"), dados->avioes[i].velocidade);
	}
	_tprintf(_T("Lista de Passageiros\n"));
	for (int i = 0; i < dados->n_passageiros_atuais; i++) {
		_tprintf(_T("Passageiro %d\n"), i);
		_tprintf(_T("\tNome: %s\n"), dados->passageiros[i].nome);
		_tprintf(_T("\tOrigem: %s\n"), dados->passageiros[i].origem);
		_tprintf(_T("\tDestino: %s\n"), dados->passageiros[i].destino);
		if (dados->passageiros->aviao != NULL) {
			_tprintf(_T("\tA Bordo do avião: %s\n"), dados->passageiros[i].aviao->id_processo);
		}

	}
	ReleaseMutex(dados->mutex_acede_aeroportos);
	ReleaseMutex(dados->mutex_acede_avioes);
	ReleaseMutex(dados->mutex_acede_passageiros);
}

void suspendeAvioes(struct_dados* dados) {
	int cont = MAX_AVIOES - dados->n_avioes_atuais;

	for (int i = 0; i < cont; i++) {
		WaitForSingleObject(dados->semafAvioesAtuais, INFINITE);
	}
}

void retomaAvioes(struct_dados* dados) {
	int cont = MAX_AVIOES - dados->n_avioes_atuais;

	ReleaseSemaphore(dados->semafAvioesAtuais, cont, NULL);
}

BOOL EmbarcaPassageirosSePossivel(struct_dados* dados) {
	struct_msg_control_passageiro msg;		
	WaitForSingleObject(dados->mutex_acede_passageiros, INFINITE);
	for (int i = 0; i < dados->n_passageiros_atuais; i++) {

		if (dados->passageiros[i].aviao == NULL) {
			dados->passageiros[i].aviao=getAviaoComOrigemDestino(dados, dados->passageiros[i].origem, dados->passageiros[i].destino);
			if (dados->passageiros[i].aviao != NULL) {
				dados->passageiros[i].aviao->nr_passageiros_atuais++;
				WaitForSingleObject(dados->mutex_resposta_passageiro, INFINITE);
				PreencheResposta(dados, &msg, AVIAO_ATRIBUIDO, dados->passageiros[i].id_processo);
				RespondeAoPassageiro(dados, &msg, dados->passageiros[i].id_processo);
				ReleaseMutex(dados->mutex_resposta_passageiro);
			}
		}

	}		
	ReleaseMutex(dados->mutex_acede_passageiros);
}

struct_aviao* getAviaoComOrigemDestino(struct_dados* dados, struct_aeroporto* origem, struct_aeroporto* destino) {
	for (int i = 0; i < dados->n_avioes_atuais; i++) {
		if (verificaAviaoEmVoo(&dados->avioes[i]) == FALSE && dados->avioes[i].lotacao > dados->avioes[i].nr_passageiros_atuais) {
			if (dados->avioes[i].origem == origem && dados->avioes[i].destino == destino) {
				return &dados->avioes[i];
			}
		}
	}
	return NULL;
}

BOOL verificaAviaoEmVoo(struct_aviao* aviao) {
	if (aviao->pos_x == aviao->origem->pos_x && aviao->pos_y == aviao->origem->pos_y) {
		return FALSE;
	}
	return TRUE;
}

void InformaNovasCoordenadasPassageiro(struct_dados* dados, struct_aviao* aviao) {
	struct_msg_control_passageiro msg;
	for (int i = 0; i < dados->n_passageiros_atuais; i++) {
		WaitForSingleObject(dados->mutex_acede_passageiros, INFINITE);
		if (dados->passageiros[i].aviao == aviao) {
			WaitForSingleObject(dados->mutex_resposta_passageiro, INFINITE);
			PreencheResposta(dados, &msg, NOVA_COORDENADA, dados->passageiros[i].id_processo);
			RespondeAoPassageiro(dados, &msg, dados->passageiros[i].id_processo);
			ReleaseMutex(dados->mutex_resposta_passageiro);
		}
		ReleaseMutex(dados->mutex_acede_passageiros);
	}
}

void InformaAviaoDespenhado(struct_dados* dados, struct_aviao* aviao) {
	struct_msg_control_passageiro msg;
	for (int i = 0; i < dados->n_passageiros_atuais; i++) {
		WaitForSingleObject(dados->mutex_acede_passageiros, INFINITE);
		if (dados->passageiros[i].aviao == aviao) {
			WaitForSingleObject(dados->mutex_resposta_passageiro, INFINITE);
			PreencheResposta(dados, &msg, AVIAO_DESPENHOU, dados->passageiros[i].id_processo);
			RespondeAoPassageiro(dados, &msg, dados->passageiros[i].id_processo);
			ReleaseMutex(dados->mutex_resposta_passageiro);
		}
		ReleaseMutex(dados->mutex_acede_passageiros);
	}
}

//Funcao de encerrar o control
void encerrar(struct_dados* dados) {
	HANDLE eventoEncerraControl = CreateEvent(
		NULL,            //LPSECURITY_ATTRIBUTES lpEventAttributes,
		TRUE,            //BOOL bManualReset, reset MANUAL
		FALSE,            //BOOL bInitialState, FALSE = bloqueante/não sinalizado
		EVENTO_ENCERRA_CONTROL              //LPCSTR lpName
	);
	if (eventoEncerraControl == NULL) {
		_tprintf(TEXT("Erro ao criar o evento de encerrar\n"));
		return;
	}

	SetEvent(eventoEncerraControl);
	CloseHandle(eventoEncerraControl);
}



