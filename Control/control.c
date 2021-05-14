#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "estruturas.h"
#define NTHREADS 2


//Declaracao de Funcoes e Threads
DWORD WINAPI Menu(LPVOID param);
DWORD WINAPI Comunicacao(LPVOID param);
void RespondeAoAviao(struct_dados* dados, struct_aviao_com* comunicacaoGeral);
BOOL CriaAeroporto(TCHAR nome[TAM], int x, int y, struct_dados* dados);
void Lista(struct_dados* dados);
int getIndiceAviao(int id_processo, struct_dados* dados);
void preencheComunicacaoParticular(struct_dados* dados, struct_aviao_com* comunicacaoGeral, struct_controlador_com* comunicacaoParticular);
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
	DWORD resultado_chave; //inteiro long, indica o que aconteceu � chave, se foi criada ou aberta ou n�o
	TCHAR nome_chave[TAM] = _T("SOFTWARE\\TEMP\\TP_SO2"), nome_par_avioes[TAM] = _T("maxAvioes"),nome_par_aeroportos[TAM] = _T("maxAeroportos");
	DWORD par_valor_avioes = 10, par_valor_aeroportos = 10;


	
	dados.n_aeroportos_atuais = 0;
	
	CreateMutex(0, FALSE, _T("Local\\$controlador$")); // try to create a named mutex
	if (GetLastError() == ERROR_ALREADY_EXISTS) // did the mutex already exist?
	{
		_tprintf(_T("ERRO! J� existe uma inst�ncia deste programa a correr!\n"));
		return -1; // quit; mutex is released automatically
	}

	dados.mutex_comunicacao = CreateMutex(NULL, FALSE, MUTEX_COMUNICACAO_CONTROL);
	if (dados.mutex_comunicacao == NULL) {
		_tprintf(_T("Erro ao criar o mutex da primeira comunicacao!\n"));
		return -1;
	}

	//Criacao da Key onde serao guardados o max de avioes e Aeroportos
	if (RegCreateKeyEx(HKEY_CURRENT_USER,
		nome_chave /*caminho*/,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE /*op��o default, funcionaria igual com 0*/,
		KEY_ALL_ACCESS /*acesso total a todas as funcionalidades da chave*/,
		NULL /*quem pode ter acesso, neste caso apenas o proprio*/,
		&chave,
		&resultado_chave)
		!= ERROR_SUCCESS) /*diferente de sucesso*/
	{
		_tprintf(TEXT("ERRO! A chave n�o foi criada nem aberta!\n"));
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
		&par_valor_avioes, // para n�o dar warning
		sizeof(par_valor_avioes)  //tamanho do que est� escrito na string incluindo o /0
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
		&par_valor_aeroportos, // para n�o dar warning
		sizeof(par_valor_aeroportos)  //tamanho do que est� escrito na string incluindo o /0
	)!=ERROR_SUCCESS)
	{
		_tprintf(TEXT("ERRO ao aceder ao atributo %s !\n"),nome_par_aeroportos);
		return -1;
	}

	//Criacao das Threads
	hthreads[0] = CreateThread(NULL, 0, Menu, &dados, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! N�o foi poss�vel criar a thread do menu!\n"));
		return -1; 
	}
	hthreads[1] = CreateThread(NULL, 0, Comunicacao, &dados, 0, NULL);
	if (hthreads[0] == NULL) {
		_tprintf(_T("ERRO! N�o foi poss�vel criar a thread da Comunica��o!\n"));
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
		_tprintf(_T("\t-> Suspender/Ativar a aceita��o de novos avi�es (suspender <on/off>)\n"));
		_tprintf(_T("\t-> Listar todos os Aeroportos, Avi�es e Passageiros (lista)\n"));
		_tprintf(_T("\t-> Encerrar (encerrar)\n"));
		_tprintf(_T("Comando: "));
		_fgetts(com_total, TAM, stdin);
		com_total[_tcslen(com_total) - 1] = '\0';
		cont = _stscanf_s(com_total, _T("%s %s %s %s"), com, (unsigned)_countof(com), arg1,(unsigned)_countof(arg1), arg2,(unsigned)_countof(arg2), arg3,(unsigned)_countof(arg3));
		if (_tcscmp(com, _T("criar")) == 0 && cont == 4) {
			CriaAeroporto(arg1,_tstoi(arg2),_tstoi(arg3),dados);
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
						break;
					}
					else {
						_tprintf(_T("Comando Inv�lido!!!!\n"));
						fflush(stdin);
					}
				}
			}
		}

	} while (TRUE);
		/*
		* A interface deve permitir receber os seguintes comandos:
			Encerrar todo o sistema (todas as aplica��es s�o notificadas).
				Criar novos aeroportos.
				Suspender/ativar a aceita��o de novos avi�es por parte dos utilizadores.
				Listar todos os aeroportos, avi�es e passageiros
		*/

	return 0;
}


DWORD WINAPI Comunicacao(LPVOID param) {
	int id_aviao = 0;
	struct_dados* dados = (struct_dados*) param;
	struct_memoria_geral* ptrMemoriaGeral;
	HANDLE objMapGeral;
	struct_aviao_com comunicacaoGeral;
	HANDLE semafEscritos, semafLidos;
	//L� da memoria partilhada geral
	//Criacao dos semaforos
	semafEscritos = CreateSemaphore(NULL, 0, MAX_AVIOES, SEMAFORO_AVIOES);
	if (semafEscritos == NULL) {
		_tprintf(_T("Erro ao criar o sem�foro dos escritos!\n"));
		return -1;
	}
	semafLidos = CreateSemaphore(NULL, MAX_AVIOES, MAX_AVIOES, SEMAFORO_VAZIOS);
	if (semafLidos == NULL) {
		_tprintf(_T("Erro ao criar o sem�foro dos lidos!\n"));
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
		RespondeAoAviao(dados,&comunicacaoGeral);
	}
	return 0;
}

//Codigo de Funcoes

void RespondeAoAviao(struct_dados* dados, struct_aviao_com* comunicacaoGeral) { //Responde a cada aviao atraves da memoria partilhada particular
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
		return -1;
	}
	fflush(stdout);
	ptrMemoriaParticular = (struct_memoria_particular*)MapViewOfFile(objMapParticular, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
	if (ptrMemoriaParticular == NULL) {
		_tprintf(_T("Erro ao criar o ptrMemoria Paricular!\n"));
		return -1;
	}
	fflush(stdout);
	// colocar em comunicacao particular o que � pretendido enviar
	mutexParticular = CreateMutex(NULL, FALSE, mutex_aviao);
	if (mutexParticular == NULL) {
		_tprintf(_T("Erro ao criar o mutex do aviao!\n"));
		return -1;
	}
	WaitForSingleObject(mutexParticular, INFINITE);
	preencheComunicacaoParticular(dados,comunicacaoGeral,&comunicacaoParticular);
	_tprintf(_T("TIPO MSG: %d\n"), comunicacaoParticular.tipomsg);
	//WaitForSingleObject(mutexParticular, INFINITE);
	CopyMemory(&ptrMemoriaParticular->resposta[0], &comunicacaoParticular, sizeof(struct_controlador_com));
	ReleaseMutex(mutexParticular);
}

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
				_tprintf(_T("Erro! Os aeroportos devem estar a uma dist�ncia de 10 posi��es!!!\n"));
				return FALSE;
			}
		}
		
		_tcscpy_s(dados->aeroportos[dados->n_aeroportos_atuais].nome, _countof(dados->aeroportos[dados->n_aeroportos_atuais].nome), nome);
		dados->aeroportos[dados->n_aeroportos_atuais].pos_x = x;
		dados->aeroportos[dados->n_aeroportos_atuais].pos_y = y;
		dados->n_aeroportos_atuais++;
		return TRUE;
	}
	_tprintf(_T("Erro! J� foi atingido o m�ximo de aeroportos\n"));
	return FALSE;
}

void Lista(struct_dados* dados) {
	_tprintf(_T("Lista de Aeroportos\n"));
	for (int i = 0; i < dados->n_aeroportos_atuais; i++) {
		_tprintf(_T("%s\n"),dados->aeroportos[i].nome);
		_tprintf(_T("\tCoordenada X: %d\n"),dados->aeroportos[i].pos_x);
		_tprintf(_T("\tCoordenada Y: %d\n"),dados->aeroportos[i].pos_y);
	}
	_tprintf(_T("Lista de Avioes\n"));
	for (int i = 0; i < dados->n_avioes_atuais; i++) {
		_tprintf(_T("Aviao: %d %d\n"), i, dados->avioes[i].id_processo);
		_tprintf(_T("\tCapacidade: %d\n"), dados->avioes[i].lotacao);
		_tprintf(_T("\tDestino: %s\n"), dados->avioes[i].destino->nome);
		_tprintf(_T("\tVelocidade: %d\n"), dados->avioes[i].velocidade);
	}

}

int getIndiceAviao(int id_processo, struct_dados* dados) {
	for (int i = 0; i < dados->n_avioes_atuais; i++) {
		if (dados->avioes[i].id_processo == id_processo) {
			return i;
		}
	}
	return -1;
}

void preencheComunicacaoParticular(struct_dados* dados, struct_aviao_com* comunicacaoGeral, struct_controlador_com* comunicacaoParticular) {
	int indiceAeroporto;
	switch (comunicacaoGeral->tipomsg)
	{
	case NOVO_AVIAO:
		indiceAeroporto = getIndiceAeroporto(dados,comunicacaoGeral->nome_origem);
		if (indiceAeroporto == -1) {
			comunicacaoParticular->tipomsg = AVIAO_RECUSADO;
			return;
		}
		else {
			comunicacaoParticular->tipomsg = AVIAO_CONFIRMADO;
			comunicacaoParticular->x_origem = dados->aeroportos[indiceAeroporto].pos_x;
			comunicacaoParticular->y_origem = dados->aeroportos[indiceAeroporto].pos_y;
			return;
		}
		break;
	case NOVO_DESTINO:
		break;
	case NOVAS_COORDENADAS:
		break;
	case CHEGADA_AO_DESTINO:
		break;
	case ENCERRAR_AVIAO:
		break;
	}
}

int getIndiceAeroporto(struct_dados* dados,  TCHAR aeroporto[]) {
	for (int i = 0; i < dados->n_aeroportos_atuais; i++) {
		if (_tcscmp(dados->aeroportos[i].nome, aeroporto) == 0) {
			return i;
		}
	}
	return -1;
}