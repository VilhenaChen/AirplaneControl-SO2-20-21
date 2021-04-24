#pragma once
#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#define TAM 200
#define TAM_MAP 1000

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
} struct_aviao;