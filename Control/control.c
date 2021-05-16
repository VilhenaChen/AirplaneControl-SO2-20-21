#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "estruturas.h"
#define NTHREADS 2
#define MUTEX_ACEDER_AVIOES _T("Mutex para aceder a estrutura dos avioes")
#define MUTEX_ACEDER_AEROPORTOS _T("Mutex para aceder a estrutura dos aeroportos")

//Estrutura onde sao guardados os dados do control
typedef struct {
	struct_aeroporto aeroportos[MAX_AEROPORTOS];
	struct_aviao avioes[MAX_AVIOES];
	//struct_memoria_geral* ptrMemoriaGeral;
	//HANDLE objMapGeral;
	int n_aeroportos_atuais;
	int n_avioes_atuais;
	HANDLE mutex_comunicacao;
	HANDLE mutex_acede_avioes;
	HANDLE mutex_acede_aeroportos;
} struct_dados;

//Declaracao de Funcoes e Threads
DWORD WINAPI Menu(LPVOID param);
DWORD WINAPI Comunicacao(LPVOID param);
BOOL RespondeAoAviao(struct_dados* dados, struct_aviao_com* comunicacaoGeral);
void preencheInformacoesSemResposta(struct_dados* dados, struct_aviao_com* comunicacaoGeral);
BOOL CriaAeroporto(TCHAR nome[TAM], int x, int y, struct_dados* dados);
void InsereAviao(struct_dados* dados, int idProcesso, int capacidade, int velocidade, int indiceAeroporto);
void EliminaAviao(struct_dados* dados, int idProcesso);
void Lista(struct_dados* dados);
int getIndiceAviao(int id_processo, struct_dados* dados);
void preencheComunicacaoParticularEAtualizaInformacoes(struct_dados* dados, struct_aviao_com* comunicacaoGeral, struct_controlador_com* comunicacaoParticular);
int getIndiceAeroporto(struct_dados* dados, TCHAR aeroporto[]);


int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif
	
	HANDLE hthreads[NTHREADS];
	struct_dados dados;
	HKEY chave; //Hanle para a chave depois de criada/aberta
	DWORD resultado_chave; //inteiro long, indica o que aconteceu à chave, se foi criada ou aberta ou não
	TCHAR nome_chave[TAM] = _T("SOFTWARE\\TEMP\\TP_SO2"), nome_par_avioes[TAM] = _T("maxAvioes"),nome_par_aeroportos[TAM] = _T("maxAeroportos");
	DWORD par_valor_avioes = 10, par_valor_aeroportos = 10;


	
	dados.n_aeroportos_atuais = 0;
	dados.n_avioes_atuais = 0;
	
	CreateMutex(0, FALSE, _T("Local\\$controlador$")); // try to create a named mutex
	if (GetLastError() == ERROR_ALREADY_EXISTS) // did the mutex already exist?
	{
		_tprintf(_T("ERRO! Já existe uma instância deste programa a correr!\n"));
		return -1; // quit; mutex is released automatically
	}

	dados.mutex_comunicacao = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_CONTROL);
	if (dados.mutex_comunicacao == NULL) {
		_tprintf(_T("Erro ao criar o mutex da primeira comunicacao!\n"));
		return -1;
	}

	dados.mutex_acede_avioes = CreateMutex(NULL, FALSE, MUTEX_ACEDER_AVIOES);
	if (dados.mutex_acede_avioes == NULL) {
		_tprintf(_T("Erro ao criar o mutex de aceder aos avioes!\n"));
		return -1;
	}

	dados.mutex_acede_aeroportos = CreateMutex(NULL, FALSE, MUTEX_ACEDER_AEROPORTOS);
	if (dados.mutex_acede_aeroportos == NULL) {
		_tprintf(_T("Erro ao criar o mutex de aceder aos aeroportos!\n"));
		return -1;
	}

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
		return -1;
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
		return -1;
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
		return -1;
	}

	//Criacao das Threads
	hthreads[0] = CreateThread(NULL, 0, Menu, &dados, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread do menu!\n"));
		return -1; 
	}
	hthreads[1] = CreateThread(NULL, 0, Comunicacao, &dados, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! Não foi possível criar a thread da Comunicação!\n"));
		return -1; 
	}

	WaitForMultipleObjects(NTHREADS, hthreads, FALSE, INFINITE);

	for (int i = 0; i < NTHREADS; i++) {
		CloseHandle(hthreads[i]);
	}

	RegCloseKey(chave);

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

			}
			else {
				if (_tcscmp(com, _T("lista")) == 0) {
					Lista(dados);
				}
				else {
					if (_tcscmp(com, _T("encerrar")) == 0) {
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
		/*
		* A interface deve permitir receber os seguintes comandos:
			Encerrar todo o sistema (todas as aplicações são notificadas).
				Criar novos aeroportos.
				Suspender/ativar a aceitação de novos aviões por parte dos utilizadores.
				Listar todos os aeroportos, aviões e passageiros
		*/

	return 0;
}

//Thread Comunicação
DWORD WINAPI Comunicacao(LPVOID param) {
	int id_aviao = 0;
	struct_dados* dados = (struct_dados*) param;
	struct_memoria_geral* ptrMemoriaGeral;
	HANDLE objMapGeral;
	struct_aviao_com comunicacaoGeral;
	HANDLE semafEscritos, semafLidos;
	//Lê da memoria partilhada geral
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
	//Verificar se nao e NULL
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
	return 0;
}

//Codigo de Funcoes

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
	//Verificar se nao e NULL
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
	_tprintf(_T("TIPO MSG: %d\n"), comunicacaoParticular.tipomsg);
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
			ReleaseMutex(dados->mutex_acede_aeroportos);

			ReleaseMutex(dados->mutex_acede_avioes);
			return;
		}
		break;
	}
}

void preencheInformacoesSemResposta(struct_dados* dados, struct_aviao_com* comunicacaoGeral) {
	int indiceAviao = -1;
	switch (comunicacaoGeral->tipomsg)
	{
	case NOVAS_COORDENADAS:
		WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
		indiceAviao = getIndiceAviao(comunicacaoGeral->id_processo, dados);
		dados->avioes[indiceAviao].pos_x = comunicacaoGeral->pos_x;
		dados->avioes[indiceAviao].pos_y = comunicacaoGeral->pos_y;
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
		_tprintf(_T("Avião: %d - %d --- Aterrou no Aeroporto de %s\n"), indiceAviao, dados->avioes[indiceAviao].id_processo, dados->avioes[indiceAviao].origem->nome);
		ReleaseMutex(dados->mutex_acede_avioes);
		break;
	case ENCERRAR_AVIAO:
		EliminaAviao(dados, comunicacaoGeral->id_processo);
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
	_tprintf(_T("INDICE A APAGAR: %d"), indiceApagar);
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
		}
		else {
			dados->avioes[i].id_processo = 0;
			dados->avioes[i].lotacao = 0;
			dados->avioes[i].origem = NULL;
			dados->avioes[i].pos_x = 0;
			dados->avioes[i].pos_y = 0;
			dados->avioes[i].velocidade = 0;
			dados->avioes[i].destino = NULL;
		}
	}
	n_avioes--;
	dados->n_avioes_atuais = n_avioes;

}

//Funções lista
void Lista(struct_dados* dados) {
	WaitForSingleObject(dados->mutex_acede_avioes, INFINITE);
	WaitForSingleObject(dados->mutex_acede_aeroportos, INFINITE);
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
	ReleaseMutex(dados->mutex_acede_aeroportos);
	ReleaseMutex(dados->mutex_acede_avioes);
}





