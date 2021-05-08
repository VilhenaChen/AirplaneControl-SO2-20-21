#pragma once
#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#define TAM 200
#define TAM_MAP 1000
#define MAX_AVIOES 10
#define MAX_AEROPORTOS 2
#define MEMORIA_CONTROL _T("Memoria Control")
#define MEMORIA_AVIAO _T("Memoria avião %d")
#define MUTEX_AVIAO _T("Mutex do Aviao %d")
#define SEMAFORO_AVIOES _T("Semaforo dos Avioes")
#define SEMAFORO_VAZIOS _T("Semaforo dos Vazios")
#define MUTEX_CONTROLADOR _T("Mutex do Controlador")


typedef struct {
	TCHAR nome[TAM];
	int pos_x;
	int pos_y;
} struct_aeroporto;

typedef struct {
	int id;
	int lotacao;
	int velocidade;
	int pos_x;
	int pos_y;
	struct_aeroporto* origem;
	struct_aeroporto* destino;
	HANDLE mutex;
} struct_aviao;

typedef struct {
	struct_aeroporto aeroportos[MAX_AEROPORTOS];
	struct_aviao avioes[MAX_AVIOES];
	//struct_memoria_geral* ptrMemoriaGeral;
	//HANDLE objMapGeral;
	int n_aeroportos_atuais;
	int n_avioes_atuais;
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
	HANDLE mutex;
}  struct_aviao_com;

typedef struct {
	int x_origem;
	int y_origem;
	int x_destino;
	int y_destino;
	BOOL avancar;
	BOOL encerrar;
}  struct_controlador_com;

typedef struct {
	struct_aviao_com aviao;
} struct_memoria_particular;

typedef struct {
	struct_controlador_com controlador[TAM];
	int in_control;
	int out_control;
	int nrControladores;
	int nrAvioes;
} struct_memoria_geral;

