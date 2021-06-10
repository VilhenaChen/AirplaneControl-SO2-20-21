#pragma once
#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

//DEFINES TAMANHOS E MAXMOS
#define TAM 200
#define TAM_MAP 1000
#define MAX_AVIOES 10
#define MAX_AEROPORTOS 10
#define MAX_PASSAGEIROS 200

//DEFINES PARA NOMES DE MUTEXES, SEMAFOROS, ETC.
#define MEMORIA_CONTROL _T("Memoria Control")
#define MEMORIA_AVIAO _T("Memoria aviao %d")
#define MUTEX_AVIAO _T("Mutex do Aviao %d") //Comunicacao entre Controlador e Aviao
#define SEMAFORO_AVIOES _T("Semaforo dos Avioes")
#define SEMAFORO_VAZIOS _T("Semaforo dos Vazios")
#define SEMAFORO_AVIOES_ATIVOS _T("Semaforo dos Avioes Ativos")
#define MUTEX_COMUNICACAO_CONTROL _T("Mutex da Primeira Comunicacao Control")
#define MUTEX_COMUNICACAO_AVIAO _T("Mutex das Comunicacoes dos Avioes") // Bloqueio da escrita dos avioes para o control quando um esta a escrever
#define EVENTO_ENCERRA_CONTROL _T("Evento encerra Control")
#define MUTEX_PASSAGEIROS _T("Mutex dos Passageiros")
#define MUTEX_PIPE_CONTROL _T("Mutex Pipe Control")
#define MUTEX_PIPE_PASSAGEIRO _T("Mutex Pipe Passageiro")

//DEFINES PARA TIPOS DE MENSAGENS AVIAO-CONTROLADOR
#define NOVO_AVIAO 1
#define	NOVO_DESTINO 2
#define NOVAS_COORDENADAS 3
#define CHEGADA_AO_DESTINO 4
#define ENCERRAR_AVIAO 5

//DEFINES PARA TIPOS DE MENSAGENS CONTROLADOR-AVIAO
#define AVIAO_CONFIRMADO 1
#define AVIAO_RECUSADO 2
#define DESTINO_VERIFICADO 3
#define DESTINO_REJEITADO 4

//NOMES DOS PIPES
#define PIPE_CONTROL_GERAL _T("\\\\.\\pipe\\control")
#define PIPE_PASSAG_PARTICULAR _T("\\\\.\\pipe\\passag_%d")


//DEFINES PARA TIPOS DE MENSAGENS PASSAGEIRO-CONTROL
#define NOVO_PASSAGEIRO 1
#define ENCERRAR_PASSAGEIRO 2

//DEFINES PARA TIPOS DE MENSAGENS CONTROLADOR-PASSAGEIRO
#define PASSAGEIRO_ACEITE 1
#define PASSAGEIRO_RECUSADO 2
#define NOVA_COORDENADA 3
#define AVIAO_DESPENHOU 4


typedef struct {
	TCHAR nome[TAM];
	int pos_x;
	int pos_y;
	int passageiros_atuais;
} struct_aeroporto;

typedef struct {
	int id_processo;
	int lotacao;
	int velocidade;
	int pos_x;
	int pos_y;
	struct_aeroporto* origem;
	struct_aeroporto* destino;
} struct_aviao;

typedef struct {	
	TCHAR nome[TAM];
	int tempo_espera;
	struct_aeroporto* origem;
	struct_aeroporto* destino;
	struct_aviao* aviao;
	int id_processo;
} struct_passageiro; 

typedef struct {
	int tipo;
	int x_origem;
	int y_origem;
	int x_destino;
	int y_destino;
	int x_atual;
	int y_atual;
} struct_msg_control_passageiro;

typedef struct {
	int tipo;
	int id_processo;
	int tempo_espera;
	TCHAR origem[TAM];
	TCHAR destino[TAM];
	TCHAR nome[TAM];
} struct_msg_passageiro_control;

typedef struct {
	TCHAR nome_origem[TAM];
	TCHAR nome_destino[TAM];
	int pos_x;
	int pos_y;
	int id_processo;
	int lotacao;
	int velocidade;
	int tipomsg;
}  struct_aviao_com; //Aviao Control

typedef struct {
	int x_origem;
	int y_origem;
	int x_destino;
	int y_destino;
	BOOL encerrar;
	int tipomsg;
}  struct_controlador_com; //Control Aviao

typedef struct {
	struct_controlador_com resposta[1];
} struct_memoria_particular; //Resposta Control -> Aviao

typedef struct {
	struct_aviao_com coms_controlador[TAM];
	int in;
	int out;
	int nrAvioes;
} struct_memoria_geral; //Resposta Aviao -> Control





