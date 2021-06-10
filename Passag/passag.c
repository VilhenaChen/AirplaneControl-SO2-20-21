#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "../Control/estruturas.h"
#define NTHREADSPRINCIPAIS 2

typedef struct {
	struct_passageiro eu;
	HANDLE hPipeControl;
	HANDLE hMeuPipe;
	HANDLE mutex_Meu_Pipe;
	HANDLE mutex_pipe_control;
} struct_dados;


//Declaracao das funcoes e das threads
DWORD WINAPI Principal(LPVOID param);
DWORD WINAPI VerificaEncerramentoControl(LPVOID param);

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
	int indiceThread;
	_tprintf(_T("aqui1\n"));
	//Verificar se os argumentos estão a ser passados de forma correta
	if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
		_tprintf(_T("ERRO! Não foram passados os argumentos necessários ao lançamento do Passageiro!\n"));
		return -1; // quit; mutex is released automatically
	}
	_tprintf(_T("aqui2\n"));
	dados.eu.aviao = NULL;
	dados.eu.id_processo = GetCurrentProcessId();
	dados.eu.origem = &origem;
	_tcscpy_s(dados.eu.origem->nome, _countof(dados.eu.origem->nome), argv[1]);
	dados.eu.destino = &destino;
	_tcscpy_s(dados.eu.destino->nome, _countof(dados.eu.destino->nome), argv[2]);
	_tcscpy_s(dados.eu.nome, _countof(dados.eu.nome), argv[3]);
	dados.eu.tempo_espera = -1;
	if (argv[4] != NULL) {
		dados.eu.tempo_espera = _tstoi(argv[4]);
	}
	_tprintf(_T("aqui3\n"));
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
	_tprintf(_T("aqui4\n"));
	indiceThread = WaitForMultipleObjects(NTHREADSPRINCIPAIS, hthreadsPrincipais, FALSE, INFINITE);

	indiceThread = indiceThread - WAIT_OBJECT_0;

	for (int i = 0; i < NTHREADSPRINCIPAIS; i++) {
		CloseHandle(hthreadsPrincipais[i]);
	}

	return 0;
}

DWORD WINAPI Principal(LPVOID param) {
	do {
		Sleep(1000);
	} while (TRUE);
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


//Funções do Registo
BOOL Registo(struct_dados* dados) {
	//TCHAR buffer[256];
	DWORD n;
	struct_msg_passageiro_control mensagemEscrita;
	struct_msg_control_passageiro mensagemLida;
	//_stprintf_s(buffer,countof(buffer), "%d %s %s %s %d", dados->eu.id_processo, dados->eu.nome, dados->eu.origem->nome, dados->eu.destino->nome, dados->eu.tempo_espera);
	mensagemEscrita.tipo = NOVO_PASSAGEIRO;
	_tcscpy_s(mensagemEscrita.nome, _countof(mensagemEscrita.nome), dados->eu.nome);
	_tcscpy_s(mensagemEscrita.origem, _countof(mensagemEscrita.origem), dados->eu.origem->nome);
	_tcscpy_s(mensagemEscrita.destino, _countof(mensagemEscrita.destino), dados->eu.destino->nome);
	mensagemEscrita.id_processo = dados->eu.id_processo;
	mensagemEscrita.tempo_espera = dados->eu.tempo_espera;
	_tprintf(_T("aqui6\n"));
	_tprintf(TEXT("[LEITOR] Esperar pelo pipe '%s' (WaitNamedPipe)\n")
		PIPE_CONTROL_GERAL);
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

	return TRUE;
}
