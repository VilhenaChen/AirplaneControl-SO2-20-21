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
#define MAX_AEROPORTOS 2

//DEFINES PARA NOMES DE MUTEXES, SEMAFOROS, ETC.
#define MEMORIA_CONTROL _T("Memoria Control")
#define MEMORIA_AVIAO _T("Memoria aviao %d")
#define MUTEX_AVIAO _T("Mutex do Aviao %d")
#define SEMAFORO_AVIOES _T("Semaforo dos Avioes")
#define SEMAFORO_VAZIOS _T("Semaforo dos Vazios")
#define SEMAFORO_AVIOES_ATIVOS _T("Semaforo dos Avioes Ativos")
#define MUTEX_COMUNICACAO_CONTROL _T("Mutex da Primeira Comunicacao Control")
#define MUTEX_COMUNICACAO_AVIAO _T("Mutex das Comunicacoes dos Avioes")

//DEFINES PARA TIPOS DE MENSAGENS AVIAO-CONTROLADOR
#define NOVO_AVIAO 1
#define	NOVO_DESTINO 2
#define NOVAS_COORDENADAS 3
#define CHEGADA_AO_DESTINO 4
#define ENCERRAR_AVIAO 5

//DEFINES PARA TIPOS DE MENSAGENS CONTROLADOR-AVIAO
#define AVIAO_CONFIRMADO 0
#define AVIAO_RECUSADO 1
#define DESTINO_VERIFICADO 2
#define DESTINO_REJEITADO 3
#define ENCERRAR_CONTROLADOR 4


typedef struct {
	TCHAR nome[TAM];
	int pos_x;
	int pos_y;
} struct_aeroporto;

typedef struct {
	int id_processo;
	int lotacao;
	int velocidade;
	int pos_x;
	int pos_y;
	struct_aeroporto* origem;
	struct_aeroporto* destino;
	//HANDLE mutex;
} struct_aviao;

typedef struct {
	struct_aeroporto aeroportos[MAX_AEROPORTOS];
	struct_aviao avioes[MAX_AVIOES];
	//struct_memoria_geral* ptrMemoriaGeral;
	//HANDLE objMapGeral;
	int n_aeroportos_atuais;
	int n_avioes_atuais;
	HANDLE mutex_comunicacao;
} struct_dados;

typedef struct {
	TCHAR nome_origem[TAM];
	TCHAR nome_destino[TAM];
	int pos_x;
	int pos_y;
	int prox_x;
	int prox_y;
	int id_processo;
	int lotacao;
	int velocidade;
	int tipomsg;
}  struct_aviao_com;

typedef struct {
	int x_origem;
	int y_origem;
	int x_destino;
	int y_destino;
	BOOL encerrar;
	int tipomsg;
}  struct_controlador_com;

typedef struct {
	struct_controlador_com resposta[1];
} struct_memoria_particular;

typedef struct {
	struct_aviao_com coms_controlador[TAM];
	int in;
	int out;
	int nrAvioes;
} struct_memoria_geral;

typedef struct {
	int x;
	int y;
} struct_posicoes_ocupadas;

typedef struct {
	struct_controlador_com coms_controlador[MAX_AVIOES];
} struct_memoria_mapa;

