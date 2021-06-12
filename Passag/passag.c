#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "../Control/estruturas.h"
#define NTHREADSPRINCIPAIS 2
#define NTHREADSSECUNDARIAS 2
#define MUTEX_INPUTS_PASSAG _T("Mutex dos Inputs")
#define MUTEX_FLAG_SAIR_PASSAG _T("Mutex da flag de sair")

typedef struct {
	HANDLE eventoInput;
	TCHAR com_total[TAM];
} struct_input;

typedef struct {
	struct_passageiro eu;
	HANDLE hPipeControl;
	HANDLE hMeuPipe;
	HANDLE mutex_Meu_Pipe;
	HANDLE mutex_pipe_control;
	HANDLE mutex_input;
	HANDLE mutex_flag_sair;
	struct_input input;
} struct_dados;



//Declaracao das funcoes e das threads
DWORD WINAPI Principal(LPVOID param);
DWORD WINAPI VerificaEncerramentoControl(LPVOID param);
DWORD WINAPI LeInformacoesControl(LPVOID param);
DWORD WINAPI VerificaEncerramentoPassageiro(LPVOID param);
BOOL Registo(struct_dados* dados);
void InicializaDados(struct_dados* dados, struct_aeroporto* origem, struct_aeroporto* destino, struct_aviao* aviao);
BOOL VerificaChegadaDestino(struct_dados* dados);
void FechaHandles(struct_dados* dados);
void Encerrar(struct_dados* dados);

int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	// recebe origem destino nome tempo_espera(opcional)
	// Encerra quando chegar ao destino ou quando passar o tempo maximo para embarque
	// Pode desligar-se a qualquer momento, deixando de existir
	// Comunica apenas e so com o controlador atraves de named Pipes
	HANDLE hthreadsPrincipais[NTHREADSPRINCIPAIS];
	struct_dados dados;
	struct_aeroporto origem;
	struct_aeroporto destino;
	struct_aviao aviao;
	int indiceThread;
	//Verificar se os argumentos estão a ser passados de forma correta
	if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
		_tprintf(_T("ERRO! Não foram passados os argumentos necessários ao lançamento do Passageiro!\n"));
		return -1; // quit; mutex is released automatically
	}
	InicializaDados(&dados, &origem, &destino, &aviao);
	_tcscpy_s(dados.eu.origem->nome, _countof(dados.eu.origem->nome), argv[1]);
	_tcscpy_s(dados.eu.destino->nome, _countof(dados.eu.destino->nome), argv[2]);
	_tcscpy_s(dados.eu.nome, _countof(dados.eu.nome), argv[3]);
	if (argv[4] != NULL) {
		dados.eu.tempo_espera = _tstoi(argv[4]);
	}

	Registo(&dados);

	//Criacao das Threads
	hthreadsPrincipais[0] = CreateThread(NULL, 0, Principal, &dados, 0, NULL);
	if (hthreadsPrincipais[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread Principal!\n"));
		return -1;
	}
	hthreadsPrincipais[1] = CreateThread(NULL, 0, VerificaEncerramentoControl, &dados, 0, NULL);
	if (hthreadsPrincipais[1] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread da Verificação do Encerramento do Control!\n"));
		return -1;
	}
	indiceThread = WaitForMultipleObjects(NTHREADSPRINCIPAIS, hthreadsPrincipais, FALSE, INFINITE);


	indiceThread = indiceThread - WAIT_OBJECT_0;

	for (int i = 0; i < NTHREADSPRINCIPAIS; i++) {
		CloseHandle(hthreadsPrincipais[i]);
	}

	if (indiceThread == 0) {
		Encerrar(&dados);
	}
	
	_tprintf(_T("To aqui\n"));
	FechaHandles(&dados);
	_tprintf(_T("To aqui\n"));
	return 0;
}

DWORD WINAPI Principal(LPVOID param) {
	struct_dados* dados = (struct_dados*)param;
	HANDLE hThreadsSecundarias[NTHREADSSECUNDARIAS];

	hThreadsSecundarias[0] = CreateThread(NULL, 0, LeInformacoesControl, dados, 0, NULL);
	if (hThreadsSecundarias[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread da leitura das informações do control!\n"));
		return -1;
	}
	hThreadsSecundarias[1] = CreateThread(NULL, 0, VerificaEncerramentoPassageiro, dados, 0, NULL);
	if (hThreadsSecundarias[1] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread de verificar o encerramento do Passageiro!\n"));
		return -1;
	}

	WaitForMultipleObjects(NTHREADSSECUNDARIAS,hThreadsSecundarias,FALSE,INFINITE);
	for (int i = 0; i < NTHREADSSECUNDARIAS; i++) {
		CloseHandle(hThreadsSecundarias[i]);
	}
	return 0;
}

DWORD WINAPI VerificaEncerramentoControl(LPVOID param) {
	struct_dados* dados = (struct_dados*)param;
	HANDLE eventoEncerraControl = CreateEvent(
		NULL,            //LPSECURITY_ATTRIBUTES lpEventAttributes,
		TRUE,            //BOOL bManualReset, reset MANUAL
		FALSE,            //BOOL bInitialState, FALSE = bloqueante/não sinalizado
		EVENTO_ENCERRA_CONTROL          //LPCSTR lpName
	);
	if (eventoEncerraControl == NULL) {
		_tprintf(TEXT("Erro ao criar o evento de encerrar\n"));
		return -1;
	}
	WaitForSingleObject(eventoEncerraControl, INFINITE);
	CloseHandle(eventoEncerraControl);
	return 0;
}

DWORD WINAPI LeInformacoesControl(LPVOID param) {
	struct_dados* dados = (struct_dados*)param;
	struct_msg_control_passageiro mensagemLida;
	BOOL retorno;
	DWORD n;
	do {
		//leitura do pipe do passageiro
		_tprintf(TEXT(" Esperar ligação do control... (ConnectNamedPipe)\n"));
		if (!ConnectNamedPipe(dados->hMeuPipe, NULL)) {
			_tprintf(TEXT("[ERRO] Ligação ao leitor! (ConnectNamedPipe\n"));
			exit(-1);
		}
		retorno = ReadFile(dados->hMeuPipe, &mensagemLida, sizeof(struct_msg_control_passageiro), &n, NULL);
		if (!retorno || !n) {
			_tprintf(TEXT("[Passageiro] %d %d... (ReadFile)\n"), retorno, n);
			return FALSE;
		}
		_tprintf(TEXT("[Passageiro] Recebi %d bytes: '%d'... (ReadFile)\n"), n, mensagemLida.tipo);


		_tprintf(TEXT("[ESCRITOR] Desligar o pipe (DisconnectNamedPipe)\n"));
		if (!DisconnectNamedPipe(dados->hMeuPipe)) {
			_tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
			exit(-1);
		}

		switch (mensagemLida.tipo) {
		case AVIAO_ATRIBUIDO:
			dados->eu.aviao->id_processo = mensagemLida.id_processo;
			_tprintf(_T("Embarcou no avião %d\n"), dados->eu.aviao->id_processo);
			break;
		case NOVA_COORDENADA:
			dados->eu.aviao->pos_x = mensagemLida.x_atual;
			dados->eu.aviao->pos_y = mensagemLida.y_atual;
			if (VerificaChegadaDestino(dados) == TRUE) {
				_tprintf(_T("Chegou ao Seu Destino! Encontra-se agora em %s\n"), dados->eu.destino->nome);
				_tprintf(_T("Obrigado por voar connosco\n"));
				return 0;
			}		
			_tprintf(_T("Avançou para as coordenadas x: %d y: %d\n"), dados->eu.aviao->pos_x, dados->eu.aviao->pos_y);
			break;
		case AVIAO_DESPENHOU:
			_tprintf(_T("Ups, o seu Avião despenhou-se!\n"));
			return 0;
			break;
		}
	} while (1);
	return 0;
}

DWORD WINAPI VerificaEncerramentoPassageiro(LPVOID param) {
	TCHAR com_total[TAM];
	com_total[0] = '\0';
	struct_dados* dados = (struct_dados*)param;
	do {
		//WaitForSingleObject(dados->mutex_input,INFINITE);
		_fgetts(com_total, TAM, stdin);
		com_total[_tcslen(com_total) - 1] = '\0';
	} while ((_tcscmp(com_total, _T("encerrar")) != 0));
	return 0;
}


//Funções do Registo
BOOL Registo(struct_dados* dados) {
	DWORD n;
	struct_msg_passageiro_control mensagemEscrita;
	struct_msg_control_passageiro mensagemLida;
	BOOL retorno;

	//preenchimento mensagem escrita
	mensagemEscrita.tipo = NOVO_PASSAGEIRO;
	_tcscpy_s(mensagemEscrita.nome, _countof(mensagemEscrita.nome), dados->eu.nome);
	_tcscpy_s(mensagemEscrita.origem, _countof(mensagemEscrita.origem), dados->eu.origem->nome);
	_tcscpy_s(mensagemEscrita.destino, _countof(mensagemEscrita.destino), dados->eu.destino->nome);
	mensagemEscrita.id_processo = dados->eu.id_processo;
	mensagemEscrita.tempo_espera = dados->eu.tempo_espera;

	WaitForSingleObject(dados->mutex_pipe_control, INFINITE);

	//ligação named pipe control e escrita para o mesmo
	_tprintf(TEXT("[LEITOR] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), PIPE_CONTROL_GERAL);
	if (!WaitNamedPipe(PIPE_CONTROL_GERAL, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_CONTROL_GERAL);
		exit(-1);
	}

	_tprintf(TEXT("[LEITOR] Ligação ao pipe do control... (CreateFile)\n"));
	dados->hPipeControl = CreateFile(PIPE_CONTROL_GERAL, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (dados->hPipeControl == NULL) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_CONTROL_GERAL);
		exit(-1);
	}
	_tprintf(TEXT("[LEITOR] Liguei-me...\n"));

	if (!WriteFile(dados->hPipeControl, &mensagemEscrita, sizeof(struct_msg_passageiro_control), &n, NULL)) {
		_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
		exit(-1);
	}
	
	//criação e leitura do pipe do passageiro
	_tprintf(TEXT(" Esperar ligação de um passageiro... (ConnectNamedPipe)\n"));
	if (!ConnectNamedPipe(dados->hMeuPipe, NULL)) {
		_tprintf(TEXT("[ERRO] Ligação ao leitor! (ConnectNamedPipe\n"));
		exit(-1);
	}
	retorno = ReadFile(dados->hMeuPipe, &mensagemLida, sizeof(struct_msg_control_passageiro), &n, NULL);
	if (!retorno || !n) {
		_tprintf(TEXT("[Passageiro] %d %d... (ReadFile)\n"), retorno, n);
		return FALSE;
	}
	_tprintf(TEXT("[Passageiro] Recebi %d bytes: '%d'... (ReadFile)\n"), n, mensagemLida.tipo);


	_tprintf(TEXT("[ESCRITOR] Desligar o pipe (DisconnectNamedPipe)\n"));
	if (!DisconnectNamedPipe(dados->hMeuPipe)) {
		_tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
		exit(-1);
	}
	CloseHandle(dados->hPipeControl);

	ReleaseMutex(dados->mutex_pipe_control);

	if (mensagemLida.tipo == PASSAGEIRO_RECUSADO) {
		//Talvez meter o pq de ter sido recusado
		_tprintf(_T("Passageiro recusado pelo controlador!!\n"));
		CloseHandle(dados->hMeuPipe);
		exit(-1);
	}
	else {
		dados->eu.origem->pos_x = mensagemLida.x_origem;
		dados->eu.origem->pos_y = mensagemLida.y_origem;
		dados->eu.destino->pos_x = mensagemLida.x_destino;
		dados->eu.destino->pos_y = mensagemLida.y_destino;
	}
	_tprintf(_T("Informações do Passageiro:\n"));
	_tprintf(_T("\tNome: %s\n"),dados->eu.nome);
	_tprintf(_T("\tOrigem: %s\n"), dados->eu.origem->nome);
	_tprintf(_T("\t\tx: %d  y: %d\n"), dados->eu.origem->pos_x, dados->eu.origem->pos_y);
	_tprintf(_T("\tDestino: %s\n"), dados->eu.destino->nome);
	_tprintf(_T("\t\tx: %d  y: %d\n"), dados->eu.destino->pos_x, dados->eu.destino->pos_y);
	return TRUE;
}

//Funcao para inicializar a estrutura Dados
void InicializaDados(struct_dados* dados, struct_aeroporto* origem, struct_aeroporto* destino, struct_aviao* aviao) {
	TCHAR nomePipe[TAM];

	dados->eu.id_processo = GetCurrentProcessId();
	dados->eu.origem = origem;
	dados->eu.destino = destino;
	dados->eu.aviao = aviao;
	_stprintf_s(nomePipe, _countof(nomePipe), PIPE_PASSAG_PARTICULAR, dados->eu.id_processo);
	dados->hMeuPipe = CreateNamedPipe(nomePipe, PIPE_ACCESS_INBOUND, PIPE_WAIT |PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, 1,
		sizeof(struct_msg_control_passageiro), sizeof(struct_msg_control_passageiro), 1000, NULL);
	
	if (dados->hMeuPipe == INVALID_HANDLE_VALUE) {
		//_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)"));
		_tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError());
		exit(-1);
	}

	dados->mutex_pipe_control = CreateMutex(NULL, FALSE, MUTEX_PIPE_CONTROL);
	if (dados->mutex_pipe_control == NULL) {
		_tprintf(_T("Erro ao criar o mutex do pipe do control!\n"));
		return FALSE;
	}

	dados->mutex_input = CreateMutex(NULL, FALSE, MUTEX_INPUTS_PASSAG);
	if (dados->mutex_input == NULL) {
		_tprintf(_T("Erro ao criar o mutex de input!\n"));
		return FALSE;
	}

	dados->mutex_flag_sair = CreateMutex(NULL, FALSE, MUTEX_FLAG_SAIR_PASSAG);
	if (dados->mutex_flag_sair == NULL) {
		_tprintf(_T("Erro ao criar o mutex da flag de sair!\n"));
		return FALSE;
	}
}

BOOL VerificaChegadaDestino(struct_dados* dados) {
	if (dados->eu.aviao->pos_x == dados->eu.destino->pos_x && dados->eu.aviao->pos_y == dados->eu.destino->pos_y) {
		return TRUE;
	}
	return FALSE;
}

//Funcao para fechar os handles
void FechaHandles(struct_dados* dados) {
	//CloseHandle(dados->hMeuPipe);
	_tprintf(_T("To aqui\n"));
	CloseHandle(dados->mutex_pipe_control);
	_tprintf(_T("To aqui\n"));
}

//Funcao para encerrar o Passageiro
void Encerrar(struct_dados* dados) {
	DWORD n;
	struct_msg_passageiro_control mensagemEscrita;

	//preenchimento mensagem escrita
	mensagemEscrita.tipo = ENCERRAR_PASSAGEIRO;
	mensagemEscrita.id_processo = dados->eu.id_processo;

	WaitForSingleObject(dados->mutex_pipe_control, INFINITE);

	//ligação named pipe control e escrita para o mesmo
	_tprintf(TEXT("[LEITOR] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), PIPE_CONTROL_GERAL);
	if (!WaitNamedPipe(PIPE_CONTROL_GERAL, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_CONTROL_GERAL);
		exit(-1);
	}

	_tprintf(TEXT("[LEITOR] Ligação ao pipe do control... (CreateFile)\n"));
	dados->hPipeControl = CreateFile(PIPE_CONTROL_GERAL, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (dados->hPipeControl == NULL) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_CONTROL_GERAL);
		exit(-1);
	}
	_tprintf(TEXT("[LEITOR] Liguei-me...\n"));

	if (!WriteFile(dados->hPipeControl, &mensagemEscrita, sizeof(struct_msg_passageiro_control), &n, NULL)) {
		_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
		exit(-1);
	}
	
	CloseHandle(dados->hPipeControl);

	ReleaseMutex(dados->mutex_pipe_control);
}