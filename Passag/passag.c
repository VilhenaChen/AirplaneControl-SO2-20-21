#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "../Control/estruturas.h"
#define NTHREADSPRINCIPAIS 3
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
	HANDLE waitable_timer;
	struct_input input;
} struct_dados;



//Declaracao das funcoes e das threads
DWORD WINAPI Principal(LPVOID param);
DWORD WINAPI VerificaEncerramentoControl(LPVOID param);
DWORD WINAPI VerificaTempoEspera(LPVOID param);
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

	HANDLE hthreadsPrincipais[NTHREADSPRINCIPAIS];
	struct_dados dados;
	struct_aeroporto origem;
	struct_aeroporto destino;
	struct_aviao aviao;
	int indiceThread;
	//Verificar se os argumentos est�o a ser passados de forma correta
	if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
		_tprintf(_T("ERRO! N�o foram passados os argumentos necess�rios ao lan�amento do Passageiro!\n"));
		return -1; // quit; mutex is released automatically
	}
	if (_tcscmp(argv[1], argv[2]) == 0) {
		_tprintf(_T("ERRO! A Origem e o Destino n�o podem ser iguais!\n"));
		return -1;
	}
	if (argc > 4 && argv[4] < 0) {
		_tprintf(_T("ERRO! O tempo de espera n�o pode ser negativo!\n"));
		return -1;
	}
	InicializaDados(&dados, &origem, &destino, &aviao);
	_tcscpy_s(dados.eu.origem->nome, _countof(dados.eu.origem->nome), argv[1]);
	_tcscpy_s(dados.eu.destino->nome, _countof(dados.eu.destino->nome), argv[2]);
	_tcscpy_s(dados.eu.nome, _countof(dados.eu.nome), argv[3]);
	if (argc>4) {
		dados.eu.tempo_espera = _tstoi(argv[4]);
	}

	Registo(&dados);

	//Criacao das Threads
	hthreadsPrincipais[0] = CreateThread(NULL, 0, Principal, &dados, 0, NULL);
	if (hthreadsPrincipais[0] == NULL) {
		_tprintf(_T("ERRO! N�o foi poss�vel criar a thread Principal!\n"));
		return -1;
	}
	hthreadsPrincipais[1] = CreateThread(NULL, 0, VerificaEncerramentoControl, &dados, 0, NULL);
	if (hthreadsPrincipais[1] == NULL) {
		_tprintf(_T("ERRO! N�o foi poss�vel criar a thread da Verifica��o do Encerramento do Control!\n"));
		return -1;
	}
	hthreadsPrincipais[2] = CreateThread(NULL, 0, VerificaTempoEspera, &dados, 0, NULL);
	if (hthreadsPrincipais[2] == NULL) {
		_tprintf(_T("ERRO! N�o foi poss�vel criar a thread da Verificavao do Tempo de espera!\n"));
		return -1;
	}

	indiceThread = WaitForMultipleObjects(NTHREADSPRINCIPAIS, hthreadsPrincipais, FALSE, INFINITE);


	indiceThread = indiceThread - WAIT_OBJECT_0;

	for (int i = 0; i < NTHREADSPRINCIPAIS; i++) {
		CloseHandle(hthreadsPrincipais[i]);
	}

	if (indiceThread == 0 || indiceThread == 2) {
		Encerrar(&dados);
	}
	
	FechaHandles(&dados);
	return 0;
}

DWORD WINAPI Principal(LPVOID param) {
	struct_dados* dados = (struct_dados*)param;
	HANDLE hThreadsSecundarias[NTHREADSSECUNDARIAS];

	hThreadsSecundarias[0] = CreateThread(NULL, 0, LeInformacoesControl, dados, 0, NULL);
	if (hThreadsSecundarias[0] == NULL) {
		_tprintf(_T("ERRO! N�o foi poss�vel criar a thread da leitura das informa��es do control!\n"));
		return -1;
	}
	hThreadsSecundarias[1] = CreateThread(NULL, 0, VerificaEncerramentoPassageiro, dados, 0, NULL);
	if (hThreadsSecundarias[1] == NULL) {
		_tprintf(_T("ERRO! N�o foi poss�vel criar a thread de verificar o encerramento do Passageiro!\n"));
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
		FALSE,            //BOOL bInitialState, FALSE = bloqueante/n�o sinalizado
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

DWORD WINAPI VerificaTempoEspera(LPVOID param) {
	struct_dados* dados = (struct_dados*)param;
    LARGE_INTEGER tempo;

	if (dados->eu.tempo_espera == -1) {
		do {
			Sleep(200);
		} while (1);
		return 0;
	}

	tempo.QuadPart = -((dados->eu.tempo_espera) * 10000000LL);

    if (!SetWaitableTimer(dados->waitable_timer, &tempo, 0, NULL, NULL, 0))
    {
		_tprintf(_T("Erro ao iniciar o waitable timer!\n"));
        return 0;
    }

	WaitForSingleObject(dados->waitable_timer, INFINITE);

	_tprintf(_T("O seu tempo de espera terminou!\n"));

    return 0;
}
	
DWORD WINAPI LeInformacoesControl(LPVOID param) {
	struct_dados* dados = (struct_dados*)param;
	struct_msg_control_passageiro mensagemLida;
	BOOL retorno;
	DWORD n;
	do {
		//leitura do pipe do passageiro
		if (!ConnectNamedPipe(dados->hMeuPipe, NULL)) {
			_tprintf(TEXT("[ERRO] Liga��o ao leitor! (ConnectNamedPipe\n"));
			exit(-1);
		}
		retorno = ReadFile(dados->hMeuPipe, &mensagemLida, sizeof(struct_msg_control_passageiro), &n, NULL);
		if (!retorno || !n) {
			return FALSE;
		}

		if (!DisconnectNamedPipe(dados->hMeuPipe)) {
			_tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
			exit(-1);
		}

		switch (mensagemLida.tipo) {
		case AVIAO_ATRIBUIDO:
			dados->eu.aviao->id_processo = mensagemLida.id_processo;
			_tprintf(_T("Embarcou no avi�o %d\n"), dados->eu.aviao->id_processo);
			CancelWaitableTimer(dados->waitable_timer);
			break;
		case NOVA_COORDENADA:
			dados->eu.aviao->pos_x = mensagemLida.x_atual;
			dados->eu.aviao->pos_y = mensagemLida.y_atual;
			if (VerificaChegadaDestino(dados) == TRUE) {
				_tprintf(_T("Chegou ao Seu Destino! Encontra-se agora em %s\n"), dados->eu.destino->nome);
				_tprintf(_T("Obrigado por voar connosco\n"));
				return 0;
			}		
			_tprintf(_T("Avan�ou para as coordenadas x: %d y: %d\n"), dados->eu.aviao->pos_x, dados->eu.aviao->pos_y);
			break;
		case AVIAO_DESPENHOU:
			_tprintf(_T("Ups, o seu Avi�o despenhou-se!\n"));
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
		_fgetts(com_total, TAM, stdin);
		com_total[_tcslen(com_total) - 1] = '\0';
	} while ((_tcscmp(com_total, _T("encerrar")) != 0));
	return 0;
}


//Fun��es do Registo
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

	//liga��o named pipe control e escrita para o mesmo
	if (!WaitNamedPipe(PIPE_CONTROL_GERAL, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_CONTROL_GERAL);
		exit(-1);
	}

	dados->hPipeControl = CreateFile(PIPE_CONTROL_GERAL, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (dados->hPipeControl == NULL) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_CONTROL_GERAL);
		exit(-1);
	}

	if (!WriteFile(dados->hPipeControl, &mensagemEscrita, sizeof(struct_msg_passageiro_control), &n, NULL)) {
		_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
		exit(-1);
	}
	
	//cria��o e leitura do pipe do passageiro
	if (!ConnectNamedPipe(dados->hMeuPipe, NULL)) {
		_tprintf(TEXT("[ERRO] Liga��o ao leitor! GLE =  %d (ConnectNamedPipe\n"),GetLastError());
		exit(-1);
	}
	retorno = ReadFile(dados->hMeuPipe, &mensagemLida, sizeof(struct_msg_control_passageiro), &n, NULL);
	if (!retorno || !n) {
		return FALSE;
	}

	if (!DisconnectNamedPipe(dados->hMeuPipe)) {
		_tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
		exit(-1);
	}
	CloseHandle(dados->hPipeControl);

	ReleaseMutex(dados->mutex_pipe_control);

	if (mensagemLida.tipo == PASSAGEIRO_RECUSADO) {
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
	_tprintf(_T("Informa��es do Passageiro:\n"));
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
	dados->eu.tempo_espera = -1;
	_stprintf_s(nomePipe, _countof(nomePipe), PIPE_PASSAG_PARTICULAR, dados->eu.id_processo);
	dados->hMeuPipe = CreateNamedPipe(nomePipe, PIPE_ACCESS_INBOUND, PIPE_WAIT | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, 1,
		sizeof(struct_msg_control_passageiro), sizeof(struct_msg_control_passageiro), 1000, NULL);

	if (dados->hMeuPipe == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("Erro ao criar o meu Named Pipe failed, GLE=%d.\n"), GetLastError());
		exit(-1);
	}

	dados->mutex_pipe_control = CreateMutex(NULL, FALSE, MUTEX_PIPE_CONTROL);
	if (dados->mutex_pipe_control == NULL) {
		_tprintf(_T("Erro ao criar o mutex do pipe do control!\n"));
		return;
	}

	dados->mutex_input = CreateMutex(NULL, FALSE, MUTEX_INPUTS_PASSAG);
	if (dados->mutex_input == NULL) {
		_tprintf(_T("Erro ao criar o mutex de input!\n"));
		return;
	}

	dados->mutex_flag_sair = CreateMutex(NULL, FALSE, MUTEX_FLAG_SAIR_PASSAG);
	if (dados->mutex_flag_sair == NULL) {
		_tprintf(_T("Erro ao criar o mutex da flag de sair!\n"));
		return;
	}

	dados->waitable_timer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (NULL == dados->waitable_timer){
		_tprintf(_T("Erro ao criar o waitable timer!\n"));
		return;
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
	CloseHandle(dados->mutex_pipe_control);
}

//Funcao para encerrar o Passageiro
void Encerrar(struct_dados* dados) {
	DWORD n;
	struct_msg_passageiro_control mensagemEscrita;

	//preenchimento mensagem escrita
	mensagemEscrita.tipo = ENCERRAR_PASSAGEIRO;
	mensagemEscrita.id_processo = dados->eu.id_processo;

	WaitForSingleObject(dados->mutex_pipe_control, INFINITE);

	//liga��o named pipe control e escrita para o mesmo
	if (!WaitNamedPipe(PIPE_CONTROL_GERAL, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_CONTROL_GERAL);
		exit(-1);
	}

	dados->hPipeControl = CreateFile(PIPE_CONTROL_GERAL, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (dados->hPipeControl == NULL) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_CONTROL_GERAL);
		exit(-1);
	}

	if (!WriteFile(dados->hPipeControl, &mensagemEscrita, sizeof(struct_msg_passageiro_control), &n, NULL)) {
		_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
		exit(-1);
	}
	
	CloseHandle(dados->hPipeControl);

	ReleaseMutex(dados->mutex_pipe_control);
}