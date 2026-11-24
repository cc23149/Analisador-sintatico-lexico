/******************************************************************************
 * Compilador de um subconjunto de Pascal com geracao de codigo MEPA
 * (versao basica, Kowaltowski - Apendice 3 e Capitulo 8)
 *
 * Uso:  ./compilador [entrada.txt] [saida.mepa]
 *
 * Modelo de memoria da MEPA (resumo):
 *   - Cada rotina tem um "nivel lexico" k (programa principal = 0).
 *   - D[k] e o registrador de base (display) do nivel k.
 *   - Uma variavel e enderecada pelo par (m,n):  M[ D[m] + n ]
 *       m = nivel lexico da rotina onde a variavel foi declarada
 *       n = deslocamento dentro do registro de ativacao
 *   - Variaveis locais:   n = 0, 1, 2, ...
 *   - Parametros (n deles): o i-esimo tem n = -(n+4-i)
 *   - Resultado de funcao:  n = -(n+4)
 ******************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>


#define TRACE_ON 0
#define TRACE(...) do { if (TRACE_ON) printf(__VA_ARGS__); } while (0)

#define MAX_ID       64
#define MAX_SIMBOLOS 500
#define MAX_PARAMS   16
#define MAX_NIVEIS   32

// 1 = depois de compilar, imprime o codigo MEPA na tela
#define MOSTRAR_SAIDA_NA_TELA 1

// ================= TOKENS =================
typedef enum {

    PROGRAMA,
    VARIAVEL,
    PROCEDURE,
    FUNCTION,
    LABEL,
    GOTO,

    BEGIN_TOKEN,
    END_TOKEN,
    IF_TOKEN,
    THEN_TOKEN,
    ELSE_TOKEN,
    WHILE_TOKEN,
    DO_TOKEN,

    IDENTIFICADOR,
    NUMERO,

    MAIS,
    MENOS,
    VEZES,
    DIVIDIR,

    IGUAL,
    DIFERENTE,
    MENOR,
    MENOROUIGUAL,
    MAIOR,
    MAIOROUIGUAL,

    ATRIBUICAO,

    ABRE_PARENTESES,
    FECHA_PARENTESES,

    VIRGULA,
    PONTO_E_VIRGULA,
    DOIS_PONTOS,
    PONTO,

    FIM_DE_ARQUIVO,
    DESCONHECIDO

} Token;

// ================= GLOBAIS =================
FILE *fp;                       // arquivo Pascal (entrada)
FILE *saida;                    // arquivo MEPA (saida)
char *nomeSaida = "codigo_intermediario.txt";
Token tokenAtual;

char lexema[MAX_ID];

// ================= CONTROLE DE POSICAO =================
int linha = 1;
int coluna = 0;

int tokenLinha = 1;
int tokenColuna = 0;

// ================= TABELA DE SIMBOLOS =================

typedef enum {
    TIPO_NULO,
    TIPO_INTEGER
} TipoBase;

typedef enum {
    NAT_PROGRAMA,
    NAT_PARAM_PROGRAMA,
    NAT_TIPO,
    NAT_VARIAVEL,
    NAT_PARAMETRO,
    NAT_PARAMETRO_REF,
    NAT_PROCEDURE,
    NAT_FUNCTION,
    NAT_LABEL
} Natureza;

#define ESP_NENHUM 0
#define ESP_READ   1
#define ESP_WRITE  2

typedef struct {
    char     identificador[MAX_ID];
    TipoBase tipo;
    Natureza natureza;
    int      escopo;
    int      numParams;
    int      porRef[MAX_PARAMS];

    // ---- campos usados na geracao de codigo ----
    int      nivel;         // nivel lexico MEPA (m). Programa principal = 0
    int      deslocamento;  // deslocamento (n) de variaveis, parametros e
                            // do resultado de funcoes
    int      rotulo;        // rotulo Lk (entrada de rotinas / labels)
    int      definido;      // label: ja foi definido no codigo?
    int      especial;      // ESP_READ / ESP_WRITE (rotinas pre-definidas)
} Simbolo;

Simbolo tabela[MAX_SIMBOLOS];
int topo = 0;
int escopoAtual = 0;

// ================= GERACAO DE CODIGO: ESTADO =================
int contadorRotulos = 0;

// numLocais[k] = quantidade de variaveis locais ja declaradas na rotina de
// nivel k (serve para AMEM, DMEM, ENRT e para o deslocamento de cada var)
int numLocais[MAX_NIVEIS];

// rotinaDoNivel[k] = indice (na tabela) da rotina cujo corpo esta sendo
// compilado no nivel k. Usado para validar atribuicao ao nome da funcao.
int rotinaDoNivel[MAX_NIVEIS];

// escopo 0 = predefinidos, escopo 1 = programa principal (nivel 0)
int nivelAtual() {

    return escopoAtual - 1;
}

char* nomeNatureza(Natureza n) {

    switch (n) {
        case NAT_PROGRAMA:       return "PROGRAMA";
        case NAT_PARAM_PROGRAMA: return "PARAM_PROGRAMA";
        case NAT_TIPO:           return "TIPO";
        case NAT_VARIAVEL:       return "VARIAVEL";
        case NAT_PARAMETRO:      return "PARAMETRO";
        case NAT_PARAMETRO_REF:  return "PARAMETRO_REF";
        case NAT_PROCEDURE:      return "PROCEDURE";
        case NAT_FUNCTION:       return "FUNCTION";
        case NAT_LABEL:          return "LABEL";
    }
    return "?";
}

char* nomeTipo(TipoBase t) {

    switch (t) {
        case TIPO_INTEGER: return "integer";
        default:           return "-";
    }
}

// ================= NOME DOS TOKENS =================
char* nomeToken(Token t) {

    switch(t) {

        case PROGRAMA: return "PROGRAM";
        case VARIAVEL: return "VAR";
        case PROCEDURE: return "PROCEDURE";
        case FUNCTION: return "FUNCTION";
        case LABEL: return "LABEL";
        case GOTO: return "GOTO";

        case BEGIN_TOKEN: return "BEGIN";
        case END_TOKEN: return "END";

        case IF_TOKEN: return "IF";
        case THEN_TOKEN: return "THEN";
        case ELSE_TOKEN: return "ELSE";
        case WHILE_TOKEN: return "WHILE";
        case DO_TOKEN: return "DO";

        case IDENTIFICADOR: return "IDENTIFICADOR";
        case NUMERO: return "NUMERO";

        case MAIS: return "+";
        case MENOS: return "-";
        case VEZES: return "*";
        case DIVIDIR: return "DIV";

        case IGUAL: return "=";
        case DIFERENTE: return "<>";
        case MENOR: return "<";
        case MENOROUIGUAL: return "<=";
        case MAIOR: return ">";
        case MAIOROUIGUAL: return ">=";

        case ATRIBUICAO: return ":=";

        case ABRE_PARENTESES: return "(";
        case FECHA_PARENTESES: return ")";

        case VIRGULA: return ",";
        case PONTO_E_VIRGULA: return ";";
        case DOIS_PONTOS: return ":";
        case PONTO: return ".";

        case FIM_DE_ARQUIVO: return "EOF";

        default:
            return "DESCONHECIDO";
    }
}

// ================= SAIDA DE CODIGO MEPA =================

// Em caso de erro, o arquivo de saida (incompleto) e apagado.
void encerraComErro() {

    if (saida != NULL) {
        fclose(saida);
        remove(nomeSaida);
    }

    exit(1);
}

// Escreve o prefixo da linha: "Lk:" (se houver rotulo) ou espacos.
void prefixo(int rot) {

    char buf[16];

    if (rot > 0)
        sprintf(buf, "L%d:", rot);
    else
        buf[0] = '\0';

    fprintf(saida, "%-8s", buf);
}

// Instrucao sem rotulo.  Ex.: gera("CRVL %d,%d", m, n);
void gera(const char *fmt, ...) {

    va_list ap;

    prefixo(0);

    va_start(ap, fmt);
    vfprintf(saida, fmt, ap);
    va_end(ap);

    fprintf(saida, "\n");
}

// Instrucao com rotulo.  Ex.: geraRot(5, "NADA");
void geraRot(int rot, const char *fmt, ...) {

    va_list ap;

    prefixo(rot);

    va_start(ap, fmt);
    vfprintf(saida, fmt, ap);
    va_end(ap);

    fprintf(saida, "\n");
}

int novoRotulo() {

    return ++contadorRotulos;
}

// ================= ERROS =================
void erroLexico(char mensagem[], int c) {

    printf("\n========== ERRO LEXICO ==========\n");
    printf("Linha: %d\n", linha);
    printf("Coluna: %d\n", coluna);

    if (c > 0)
        printf("Caractere encontrado: %c\n", c);

    printf("Descricao: %s\n", mensagem);
    printf("=================================\n");

    encerraComErro();
}

void erroSintatico(char *esperado, char mensagem[]) {

    printf("\n========== ERRO SINTATICO ==========\n");
    printf("Linha: %d\n", tokenLinha);
    printf("Coluna: %d\n", tokenColuna);

    printf("Token encontrado: %s", nomeToken(tokenAtual));

    if (tokenAtual == IDENTIFICADOR || tokenAtual == NUMERO)
        printf(" (%s)", lexema);

    printf("\n");

    if (esperado != NULL)
        printf("Token esperado: %s\n", esperado);

    printf("Descricao: %s\n", mensagem);
    printf("====================================\n");

    encerraComErro();
}

void erroSemantico(char mensagem[], char nome[], int l, int c) {

    printf("\n========== ERRO SEMANTICO ==========\n");
    printf("Linha: %d\n", l);
    printf("Coluna: %d\n", c);
    printf("Identificador: %s\n", nome);
    printf("Escopo atual: %d\n", escopoAtual);
    printf("Descricao: %s\n", mensagem);
    printf("====================================\n");

    encerraComErro();
}

// ================= ANALISADOR LEXICO =================
Token analex() {

    int c;
    char buffer[MAX_ID];
    int i;

    while ((c = fgetc(fp)) != EOF) {

        coluna++;

        // ================= CONTROLE DE LINHA =================
        if (c == '\n') {

            linha++;
            coluna = 0;
            continue;
        }

        // ================= IGNORA ESPACOS =================
        if (isspace(c))
            continue;

        tokenLinha = linha;
        tokenColuna = coluna;

        // ================= COMENTARIOS =================
        if (c == '(') {

            int prox = fgetc(fp);
            coluna++;

            if (prox == '*') {

                int ant = 0;
                int fechou = 0;

                while ((c = fgetc(fp)) != EOF) {

                    coluna++;

                    if (c == '\n') {

                        linha++;
                        coluna = 0;
                    }

                    if (ant == '*' && c == ')') {

                        fechou = 1;
                        break;
                    }

                    ant = c;
                }

                if (!fechou)
                    erroLexico("comentario nao fechado", 0);

                continue;
            }

            ungetc(prox, fp);
            coluna--;

            return ABRE_PARENTESES;
        }

        // ================= IDENTIFICADORES =================
        if (isalpha(c)) {

            i = 0;

            buffer[i++] = tolower(c);

            while (isalnum(c = fgetc(fp)) || c == '_') {

                if (i >= MAX_ID - 1)
                    erroLexico("identificador muito grande", 0);

                buffer[i++] = tolower(c);
                coluna++;
            }

            buffer[i] = '\0';

            ungetc(c, fp);

            strcpy(lexema, buffer);

            if (strcmp(buffer, "program") == 0)
                return PROGRAMA;

            else if (strcmp(buffer, "var") == 0)
                return VARIAVEL;

            else if (strcmp(buffer, "procedure") == 0)
                return PROCEDURE;

            else if (strcmp(buffer, "function") == 0)
                return FUNCTION;

            else if (strcmp(buffer, "label") == 0)
                return LABEL;

            else if (strcmp(buffer, "goto") == 0)
                return GOTO;

            else if (strcmp(buffer, "begin") == 0)
                return BEGIN_TOKEN;

            else if (strcmp(buffer, "end") == 0)
                return END_TOKEN;

            else if (strcmp(buffer, "if") == 0)
                return IF_TOKEN;

            else if (strcmp(buffer, "then") == 0)
                return THEN_TOKEN;

            else if (strcmp(buffer, "else") == 0)
                return ELSE_TOKEN;

            else if (strcmp(buffer, "while") == 0)
                return WHILE_TOKEN;

            else if (strcmp(buffer, "do") == 0)
                return DO_TOKEN;

            else if (strcmp(buffer, "div") == 0)
                return DIVIDIR;

            else
                return IDENTIFICADOR;
        }

        // ================= NUMEROS =================
        else if (isdigit(c)) {

            i = 0;

            buffer[i++] = c;

            while (isdigit(c = fgetc(fp))) {

                if (i >= MAX_ID - 1)
                    erroLexico("numero muito grande", 0);

                buffer[i++] = c;
                coluna++;
            }

            buffer[i] = '\0';

            ungetc(c, fp);

            strcpy(lexema, buffer);

            return NUMERO;
        }

        // ================= OPERADORES =================
        else {

            switch (c) {

                case '+':
                    return MAIS;

                case '-':
                    return MENOS;

                case '*':
                    return VEZES;

                case '=':
                    return IGUAL;

                case '>':

                    c = fgetc(fp);
                    coluna++;

                    if (c == '=')
                        return MAIOROUIGUAL;

                    ungetc(c, fp);
                    coluna--;

                    return MAIOR;

                case '<':

                    c = fgetc(fp);
                    coluna++;

                    if (c == '=')
                        return MENOROUIGUAL;

                    else if (c == '>')
                        return DIFERENTE;

                    ungetc(c, fp);
                    coluna--;

                    return MENOR;

                case ':':

                    c = fgetc(fp);
                    coluna++;

                    if (c == '=')
                        return ATRIBUICAO;

                    ungetc(c, fp);
                    coluna--;

                    return DOIS_PONTOS;

                case ')':
                    return FECHA_PARENTESES;

                case ',':
                    return VIRGULA;

                case ';':
                    return PONTO_E_VIRGULA;

                case '.':
                    return PONTO;
            }
        }

        // ================= ERRO LEXICO =================
        erroLexico("caractere invalido", c);
    }

    tokenLinha = linha;
    tokenColuna = coluna;

    return FIM_DE_ARQUIVO;
}

// ================= AVANCA TOKEN =================
void avancaToken() {

    tokenAtual = analex();
}

// ================= CONSOME TOKEN =================
void consome(Token esperado, char mensagem[]) {

    if (tokenAtual == esperado)
        avancaToken();
    else
        erroSintatico(nomeToken(esperado), mensagem);
}

// ================= OPERACOES DA TABELA DE SIMBOLOS =================

int busca(char nome[]) {

    int i;

    for (i = topo - 1; i >= 0; i--)
        if (strcmp(tabela[i].identificador, nome) == 0)
            return i;

    return -1;
}

int insere(char nome[], Natureza nat, TipoBase tipo, int l, int c) {

    int i;

    for (i = topo - 1; i >= 0 && tabela[i].escopo == escopoAtual; i--)
        if (strcmp(tabela[i].identificador, nome) == 0)
            erroSemantico("identificador ja declarado neste escopo", nome, l, c);

    if (topo >= MAX_SIMBOLOS)
        erroSemantico("tabela de simbolos cheia", nome, l, c);

    strcpy(tabela[topo].identificador, nome);
    tabela[topo].natureza = nat;
    tabela[topo].tipo = tipo;
    tabela[topo].escopo = escopoAtual;
    tabela[topo].numParams = 0;

    tabela[topo].nivel = escopoAtual - 1;
    tabela[topo].deslocamento = 0;
    tabela[topo].rotulo = 0;
    tabela[topo].definido = 0;
    tabela[topo].especial = ESP_NENHUM;

    for (i = 0; i < MAX_PARAMS; i++)
        tabela[topo].porRef[i] = 0;

    topo++;

    return topo - 1;
}

void defineTipo(int inicio, TipoBase tipo) {

    int i;

    for (i = inicio; i < topo; i++)
        tabela[i].tipo = tipo;
}

void imprimeEscopo(int escopo) {

    int i;
    char end[32];

    printf("\n---------- TABELA DE SIMBOLOS - ESCOPO %d ----------\n", escopo);
    printf("%-16s %-16s %-9s %-8s\n", "IDENTIFICADOR", "NATUREZA", "TIPO", "ENDERECO");

    for (i = 0; i < topo; i++) {

        if (tabela[i].escopo != escopo)
            continue;

        switch (tabela[i].natureza) {

            case NAT_VARIAVEL:
            case NAT_PARAMETRO:
            case NAT_PARAMETRO_REF:
                sprintf(end, "%d,%d", tabela[i].nivel, tabela[i].deslocamento);
                break;

            case NAT_PROCEDURE:
            case NAT_FUNCTION:
            case NAT_LABEL:
                if (tabela[i].rotulo > 0)
                    sprintf(end, "L%d", tabela[i].rotulo);
                else
                    strcpy(end, "-");
                break;

            default:
                strcpy(end, "-");
        }

        printf("%-16s %-16s %-9s %-8s\n",
               tabela[i].identificador,
               nomeNatureza(tabela[i].natureza),
               nomeTipo(tabela[i].tipo),
               end);
    }
}

void entraEscopo() {

    escopoAtual++;

    if (escopoAtual >= MAX_NIVEIS)
        erroSemantico("aninhamento de rotinas profundo demais", "-",
                      tokenLinha, tokenColuna);
}

void saiEscopo() {

    int i;

    // todo label declarado precisa ter sido definido no seu bloco
    for (i = topo - 1; i >= 0 && tabela[i].escopo == escopoAtual; i--)
        if (tabela[i].natureza == NAT_LABEL && !tabela[i].definido)
            erroSemantico("label declarado mas nunca definido",
                          tabela[i].identificador, tokenLinha, tokenColuna);

    imprimeEscopo(escopoAtual);

    while (topo > 0 && tabela[topo - 1].escopo == escopoAtual)
        topo--;

    escopoAtual--;
}

int declara(Natureza nat, Token esperado, char mensagem[]) {

    char nome[MAX_ID];
    int l = tokenLinha, c = tokenColuna;

    strcpy(nome, lexema);

    consome(esperado, mensagem);

    return insere(nome, nat, TIPO_NULO, l, c);
}


int declaraNaLista(Natureza nat) {

    char nome[MAX_ID];
    int l = tokenLinha, c = tokenColuna;

    strcpy(nome, lexema);

    consome(IDENTIFICADOR, "esperava identificador");

    if (tokenAtual != VIRGULA && tokenAtual != DOIS_PONTOS)
        erroSintatico(":", "esperava : na declaracao (faltou BEGIN?)");

    return insere(nome, nat, TIPO_NULO, l, c);
}

int usaIdentificador() {

    char nome[MAX_ID];
    int l = tokenLinha, c = tokenColuna;
    int idx;

    strcpy(nome, lexema);

    consome(IDENTIFICADOR, "esperava identificador");

    idx = busca(nome);

    if (idx < 0)
        erroSemantico("identificador nao declarado", nome, l, c);

    return idx;
}

// Consome um numero que deve ser um label declarado; devolve seu indice.
int usaLabel() {

    char nome[MAX_ID];
    int l = tokenLinha, c = tokenColuna;
    int idx;

    strcpy(nome, lexema);

    consome(NUMERO, "esperava numero");

    idx = busca(nome);

    if (idx < 0 || tabela[idx].natureza != NAT_LABEL)
        erroSemantico("label nao declarado", nome, l, c);

    return idx;
}

TipoBase compila_tipo() {

    char nome[MAX_ID];
    int l = tokenLinha, c = tokenColuna;
    int idx;

    strcpy(nome, lexema);

    consome(IDENTIFICADOR, "esperava tipo");

    idx = busca(nome);

    if (idx < 0 || tabela[idx].natureza != NAT_TIPO)
        erroSemantico("tipo desconhecido", nome, l, c);

    return tabela[idx].tipo;
}

// ================= GERACAO: CARGA E ARMAZENAMENTO =================

// Empilha o VALOR da variavel/parametro idx
//   variavel ou parametro por valor -> CRVL m,n
//   parametro por referencia        -> CRVI m,n  (valor indireto)
void gera_carga(int idx) {

    if (tabela[idx].natureza == NAT_PARAMETRO_REF)
        gera("CRVI %d,%d", tabela[idx].nivel, tabela[idx].deslocamento);
    else
        gera("CRVL %d,%d", tabela[idx].nivel, tabela[idx].deslocamento);
}

// Desempilha o topo e guarda em idx
//   variavel ou parametro por valor -> ARMZ m,n
//   parametro por referencia        -> ARMI m,n
//   nome de funcao (resultado)      -> ARMZ (nivel do corpo),(desloc. do resultado)
void gera_armazenamento(int idx) {

    if (tabela[idx].natureza == NAT_PARAMETRO_REF)
        gera("ARMI %d,%d", tabela[idx].nivel, tabela[idx].deslocamento);

    else if (tabela[idx].natureza == NAT_FUNCTION)
        gera("ARMZ %d,%d", tabela[idx].nivel + 1, tabela[idx].deslocamento);

    else
        gera("ARMZ %d,%d", tabela[idx].nivel, tabela[idx].deslocamento);
}

// ================= DECLARACOES =================
void compila_expressao();
void compila_comando();
void compila_bloco();

// ================= LEITURA (read) =================
void compila_leitura() {

    char nome[MAX_ID];
    int l = tokenLinha, c = tokenColuna;
    int idx;
    Natureza nat;

    if (tokenAtual != IDENTIFICADOR)
        erroSemantico("read exige uma variavel", "read", l, c);

    strcpy(nome, lexema);
    idx = usaIdentificador();
    nat = tabela[idx].natureza;

    if (nat != NAT_VARIAVEL && nat != NAT_PARAMETRO && nat != NAT_PARAMETRO_REF)
        erroSemantico("read exige uma variavel", nome, l, c);

    if (tokenAtual != VIRGULA && tokenAtual != FECHA_PARENTESES)
        erroSemantico("read nao aceita expressao", nome, l, c);

    gera("LEIT");
    gera_armazenamento(idx);
}

// ================= CHAMADA =================
void compila_argumento(int idxRotina, int pos) {

    int l = tokenLinha, c = tokenColuna;
    char nome[MAX_ID];

    strcpy(nome, tabela[idxRotina].identificador);

    // rotinas pre-definidas (read / write): numero variavel de argumentos
    if (tabela[idxRotina].numParams < 0) {

        if (tabela[idxRotina].especial == ESP_READ) {

            compila_leitura();
        }
        else {

            compila_expressao();
            gera("IMPR");
        }
        return;
    }

    if (pos >= tabela[idxRotina].numParams)
        erroSemantico("numero de argumentos maior que o esperado", nome, l, c);

    if (tabela[idxRotina].porRef[pos]) {

        char arg[MAX_ID];
        int idx;

        if (tokenAtual != IDENTIFICADOR)
            erroSemantico("parametro por referencia exige uma variavel", nome, l, c);

        strcpy(arg, lexema);
        idx = usaIdentificador();

        if (tabela[idx].natureza != NAT_VARIAVEL &&
            tabela[idx].natureza != NAT_PARAMETRO &&
            tabela[idx].natureza != NAT_PARAMETRO_REF)
            erroSemantico("parametro por referencia exige uma variavel", arg, l, c);

        // se vier algo depois (ex.: +0, *2, comparacao), nao e variavel pura
        if (tokenAtual != VIRGULA && tokenAtual != FECHA_PARENTESES)
            erroSemantico("parametro por referencia nao aceita expressao", arg, l, c);

        // Empilha o ENDERECO da variavel.
        // Se ela mesma ja e parametro por referencia, a posicao dela
        // ja contem o endereco: basta carregar (CRVL).
        if (tabela[idx].natureza == NAT_PARAMETRO_REF)
            gera("CRVL %d,%d", tabela[idx].nivel, tabela[idx].deslocamento);
        else
            gera("CREN %d,%d", tabela[idx].nivel, tabela[idx].deslocamento);
    }
    else {

        // por valor: empilha o valor da expressao
        compila_expressao();
    }
}

void compila_argumentos(int idxRotina) {

    int pos = 0;
    char nome[MAX_ID];

    strcpy(nome, tabela[idxRotina].identificador);

    consome(ABRE_PARENTESES, "esperava (");

    if (tokenAtual != FECHA_PARENTESES) {

        compila_argumento(idxRotina, pos++);

        while (tokenAtual == VIRGULA) {

            avancaToken();
            compila_argumento(idxRotina, pos++);
        }
    }

    if (tabela[idxRotina].numParams >= 0 &&
        pos != tabela[idxRotina].numParams)
        erroSemantico("numero de argumentos diferente do esperado",
                      nome, tokenLinha, tokenColuna);

    consome(FECHA_PARENTESES, "esperava )");
}

// ================= FATOR =================
void compila_fator() {

    TRACE("Reconhecendo FATOR\n");

    if (tokenAtual == IDENTIFICADOR) {

        char nome[MAX_ID];
        int l = tokenLinha, c = tokenColuna;
        int idx = usaIdentificador();
        Natureza nat = tabela[idx].natureza;

        strcpy(nome, tabela[idx].identificador);

        if (tokenAtual == ABRE_PARENTESES) {

            TRACE("Reconhecendo chamada de funcao\n");

            if (nat != NAT_FUNCTION)
                erroSemantico("so funcao pode ser chamada em expressao", nome, l, c);

            // reserva a posicao do resultado, empilha os argumentos e chama
            gera("AMEM 1");
            compila_argumentos(idx);
            gera("CHPR L%d,%d", tabela[idx].rotulo, nivelAtual());
        }
        else if (nat == NAT_FUNCTION) {

            // funcao sem parametros, chamada sem parenteses
            if (tabela[idx].numParams != 0)
                erroSemantico("numero de argumentos diferente do esperado", nome, l, c);

            gera("AMEM 1");
            gera("CHPR L%d,%d", tabela[idx].rotulo, nivelAtual());
        }
        else if (nat == NAT_VARIAVEL || nat == NAT_PARAMETRO ||
                 nat == NAT_PARAMETRO_REF) {

            gera_carga(idx);
        }
        else {

            erroSemantico("identificador nao pode ser usado em expressao", nome, l, c);
        }
    }

    else if (tokenAtual == NUMERO) {

        gera("CRCT %d", atoi(lexema));

        avancaToken();
    }

    else if (tokenAtual == ABRE_PARENTESES) {

        avancaToken();

        compila_expressao();

        consome(FECHA_PARENTESES, "esperava )");
    }

    else {

        erroSintatico(NULL, "fator invalido");
    }
}

// ================= TERMO =================
void compila_termo() {

    Token op;

    TRACE("Reconhecendo TERMO\n");

    compila_fator();

    while (tokenAtual == VEZES || tokenAtual == DIVIDIR) {

        op = tokenAtual;

        avancaToken();
        compila_fator();

        // operacao depois dos dois operandos (notacao posfixa)
        if (op == VEZES)
            gera("MULT");
        else
            gera("DIVI");
    }
}

// ================= EXPRESSAO SIMPLES =================
void compila_expressao_simples() {

    Token op;
    int negativo = 0;

    TRACE("Reconhecendo EXPRESSAO SIMPLES\n");

    if (tokenAtual == MAIS || tokenAtual == MENOS) {

        negativo = (tokenAtual == MENOS);
        avancaToken();
    }

    compila_termo();

    if (negativo)
        gera("INVR");

    while (tokenAtual == MAIS || tokenAtual == MENOS) {

        op = tokenAtual;

        avancaToken();
        compila_termo();

        if (op == MAIS)
            gera("SOMA");
        else
            gera("SUBT");
    }
}

// ================= EXPRESSAO =================
void compila_expressao() {

    Token op;

    TRACE("Reconhecendo EXPRESSAO\n");

    compila_expressao_simples();

    if (
        tokenAtual == IGUAL ||
        tokenAtual == DIFERENTE ||
        tokenAtual == MENOR ||
        tokenAtual == MENOROUIGUAL ||
        tokenAtual == MAIOR ||
        tokenAtual == MAIOROUIGUAL
    ) {

        op = tokenAtual;

        avancaToken();

        compila_expressao_simples();

        switch (op) {
            case IGUAL:        gera("CMIG"); break;
            case DIFERENTE:    gera("CMDG"); break;
            case MENOR:        gera("CMME"); break;
            case MENOROUIGUAL: gera("CMEG"); break;
            case MAIOR:        gera("CMMA"); break;
            case MAIOROUIGUAL: gera("CMAG"); break;
            default: break;
        }
    }
}

// ================= PARAMETROS =================
void compila_grupo_parametros(int idxRotina) {

    Natureza nat = NAT_PARAMETRO;
    int inicio, i;
    TipoBase tipo;

    if (tokenAtual == VARIAVEL) {

        nat = NAT_PARAMETRO_REF;
        avancaToken();
    }

    inicio = topo;

    declaraNaLista(nat);

    while (tokenAtual == VIRGULA) {

        avancaToken();
        declaraNaLista(nat);
    }

    consome(DOIS_PONTOS, "esperava :");

    tipo = compila_tipo();

    defineTipo(inicio, tipo);

    for (i = inicio; i < topo; i++) {

        if (tabela[idxRotina].numParams >= MAX_PARAMS)
            erroSemantico("muitos parametros", tabela[i].identificador,
                          tokenLinha, tokenColuna);

        tabela[idxRotina].porRef[tabela[idxRotina].numParams++] =
            (nat == NAT_PARAMETRO_REF);
    }
}

void compila_parametros(int idxRotina) {

    int primeiro = topo;   // primeiro parametro desta rotina na tabela
    int n, j;

    TRACE("Reconhecendo PARAMETROS\n");

    consome(ABRE_PARENTESES, "esperava (");

    if (tokenAtual != FECHA_PARENTESES) {

        compila_grupo_parametros(idxRotina);

        // grupos sao separados por ';'
        while (tokenAtual == PONTO_E_VIRGULA) {

            avancaToken();
            compila_grupo_parametros(idxRotina);
        }
    }

    consome(FECHA_PARENTESES, "esperava ) ou ;");

    // Agora que n e conhecido, define o deslocamento de cada parametro:
    // o j-esimo (j = 1..n) fica em  -(n + 4 - j)
    // (acima deles, na pilha: endereco de retorno, m e D[k] salvo)
    n = topo - primeiro;

    for (j = 1; j <= n; j++)
        tabela[primeiro + j - 1].deslocamento = -(n + 4 - j);
}

// ================= LABEL =================
void compila_label() {

    int idx;

    TRACE("Reconhecendo LABEL\n");

    consome(LABEL, "esperava LABEL");

    idx = declara(NAT_LABEL, NUMERO, "esperava numero");
    tabela[idx].rotulo = novoRotulo();

    while (tokenAtual == VIRGULA) {

        avancaToken();

        idx = declara(NAT_LABEL, NUMERO, "esperava numero");
        tabela[idx].rotulo = novoRotulo();
    }

    consome(PONTO_E_VIRGULA, "esperava ;");
}

// ================= VAR =================
void compila_var() {

    TRACE("Reconhecendo declaracao VAR\n");

    consome(VARIAVEL, "esperava VAR");

    while (tokenAtual == IDENTIFICADOR) {

        int inicio = topo;
        int idx;
        TipoBase tipo;

        idx = declaraNaLista(NAT_VARIAVEL);
        tabela[idx].deslocamento = numLocais[nivelAtual()]++;

        while (tokenAtual == VIRGULA) {

            avancaToken();

            idx = declaraNaLista(NAT_VARIAVEL);
            tabela[idx].deslocamento = numLocais[nivelAtual()]++;
        }

        consome(DOIS_PONTOS, "esperava :");

        tipo = compila_tipo();

        defineTipo(inicio, tipo);

        consome(PONTO_E_VIRGULA, "esperava ;");
    }
}

// ================= PROCEDURE =================
void compila_procedure() {

    int idx;

    TRACE("Reconhecendo PROCEDURE\n");

    consome(PROCEDURE, "esperava PROCEDURE");

    idx = declara(NAT_PROCEDURE, IDENTIFICADOR, "esperava identificador");

    tabela[idx].rotulo = novoRotulo();

    entraEscopo();

    rotinaDoNivel[nivelAtual()] = idx;

    if (tokenAtual == ABRE_PARENTESES)
        compila_parametros(idx);

    consome(PONTO_E_VIRGULA, "esperava ;");

    // primeira instrucao da rotina: ENPR k  (k = nivel lexico)
    geraRot(tabela[idx].rotulo, "ENPR %d", nivelAtual());

    compila_bloco();

    // retorno: remove registro de ativacao e os n parametros
    gera("RTPR %d,%d", nivelAtual(), tabela[idx].numParams);

    saiEscopo();

    consome(PONTO_E_VIRGULA, "esperava ;");
}

// ================= FUNCTION =================
void compila_function() {

    int idx;

    TRACE("Reconhecendo FUNCTION\n");

    consome(FUNCTION, "esperava FUNCTION");

    idx = declara(NAT_FUNCTION, IDENTIFICADOR, "esperava identificador");

    tabela[idx].rotulo = novoRotulo();

    entraEscopo();

    rotinaDoNivel[nivelAtual()] = idx;

    if (tokenAtual == ABRE_PARENTESES)
        compila_parametros(idx);

    consome(DOIS_PONTOS, "esperava :");

    tabela[idx].tipo = compila_tipo();

    // o resultado fica logo abaixo dos parametros: -(n + 4)
    tabela[idx].deslocamento = -(tabela[idx].numParams + 4);

    consome(PONTO_E_VIRGULA, "esperava ;");

    geraRot(tabela[idx].rotulo, "ENPR %d", nivelAtual());

    compila_bloco();

    gera("RTPR %d,%d", nivelAtual(), tabela[idx].numParams);

    saiEscopo();

    consome(PONTO_E_VIRGULA, "esperava ;");
}

// ================= COMANDO =================
void compila_comando() {

    TRACE("Reconhecendo COMANDO\n");

    if (tokenAtual == NUMERO) {

        char nome[MAX_ID];
        int l = tokenLinha, c = tokenColuna;
        int idx;

        TRACE("Reconhecendo LABEL de comando\n");

        strcpy(nome, lexema);

        idx = usaLabel();

        if (tabela[idx].escopo != escopoAtual)
            erroSemantico("label definido fora do bloco em que foi declarado",
                          nome, l, c);

        if (tabela[idx].definido)
            erroSemantico("label definido mais de uma vez", nome, l, c);

        tabela[idx].definido = 1;

        // ENRT j,n : ao chegar aqui por um DSVR, reajusta o topo da pilha
        // para logo depois das n variaveis locais da rotina de nivel j
        geraRot(tabela[idx].rotulo, "ENRT %d,%d",
                tabela[idx].nivel, numLocais[tabela[idx].nivel]);

        consome(DOIS_PONTOS, "esperava :");

        compila_comando();
    }

    else if (tokenAtual == BEGIN_TOKEN) {

        TRACE("Reconhecendo bloco BEGIN END\n");

        avancaToken();

        compila_comando();

        while (tokenAtual == PONTO_E_VIRGULA) {

            avancaToken();
            compila_comando();
        }

        consome(END_TOKEN, "esperava END (ou ; entre comandos)");
    }

    else if (tokenAtual == IF_TOKEN) {

        int rotElse, rotFim;

        TRACE("Reconhecendo comando IF\n");

        avancaToken();

        compila_expressao();

        consome(THEN_TOKEN, "esperava THEN");

        rotElse = novoRotulo();
        gera("DSVF L%d", rotElse);

        compila_comando();

        if (tokenAtual == ELSE_TOKEN) {

            TRACE("Reconhecendo ELSE\n");

            rotFim = novoRotulo();

            gera("DSVS L%d", rotFim);
            geraRot(rotElse, "NADA");

            avancaToken();

            compila_comando();

            geraRot(rotFim, "NADA");
        }
        else {

            geraRot(rotElse, "NADA");
        }
    }

    else if (tokenAtual == WHILE_TOKEN) {

        int rotInicio, rotFim;

        TRACE("Reconhecendo comando WHILE\n");

        avancaToken();

        rotInicio = novoRotulo();
        rotFim = novoRotulo();

        geraRot(rotInicio, "NADA");

        compila_expressao();

        consome(DO_TOKEN, "esperava DO");

        gera("DSVF L%d", rotFim);

        compila_comando();

        gera("DSVS L%d", rotInicio);
        geraRot(rotFim, "NADA");
    }

    else if (tokenAtual == GOTO) {

        int idx;

        TRACE("Reconhecendo comando GOTO\n");

        avancaToken();

        idx = usaLabel();

        // DSVR p,j,k : desvia para o label p (declarado no nivel j) a
        // partir do nivel k, desfazendo os registros de base intermediarios
        gera("DSVR L%d,%d,%d", tabela[idx].rotulo, tabela[idx].nivel,
             nivelAtual());
    }

    else if (tokenAtual == IDENTIFICADOR) {

        char nome[MAX_ID];
        int l = tokenLinha, c = tokenColuna;
        int idx = usaIdentificador();
        Natureza nat = tabela[idx].natureza;

        strcpy(nome, tabela[idx].identificador);

        if (tokenAtual == ATRIBUICAO) {

            TRACE("Reconhecendo atribuicao\n");

            if (nat != NAT_VARIAVEL && nat != NAT_PARAMETRO &&
                nat != NAT_PARAMETRO_REF && nat != NAT_FUNCTION)
                erroSemantico("identificador nao pode receber atribuicao", nome, l, c);

            // f := ... so vale dentro do corpo da propria funcao f
            // (ou de uma rotina aninhada nela)
            if (nat == NAT_FUNCTION) {

                int nivelCorpo = tabela[idx].nivel + 1;

                if (nivelCorpo > nivelAtual() || rotinaDoNivel[nivelCorpo] != idx)
                    erroSemantico("atribuicao ao nome de funcao fora do seu corpo",
                                  nome, l, c);
            }

            avancaToken();

            compila_expressao();

            gera_armazenamento(idx);
        }

        else if (tokenAtual == ABRE_PARENTESES) {

            TRACE("Reconhecendo chamada de procedimento\n");

            if (nat != NAT_PROCEDURE)
                erroSemantico("so procedure pode ser chamada como comando", nome, l, c);

            compila_argumentos(idx);

            // read e write sao expandidos inline (LEIT / IMPR): sem CHPR
            if (tabela[idx].especial == ESP_NENHUM)
                gera("CHPR L%d,%d", tabela[idx].rotulo, nivelAtual());
        }

        else {

            if (nat != NAT_PROCEDURE)
                erroSintatico(":=", "esperava := ou chamada de procedure");

            // procedure chamada sem parenteses
            if (tabela[idx].especial == ESP_NENHUM) {

                if (tabela[idx].numParams != 0)
                    erroSemantico("numero de argumentos diferente do esperado",
                                  nome, l, c);

                gera("CHPR L%d,%d", tabela[idx].rotulo, nivelAtual());
            }
        }
    }

    else if (tokenAtual == PONTO_E_VIRGULA ||
             tokenAtual == END_TOKEN ||
             tokenAtual == ELSE_TOKEN) {

    }

    else {

        erroSintatico(NULL, "comando invalido");
    }
}

// ================= BLOCO =================
void compila_bloco() {

    int nivel = nivelAtual();
    int rotPula;

    TRACE("Reconhecendo BLOCO\n");

    numLocais[nivel] = 0;

    if (tokenAtual == LABEL)
        compila_label();

    if (tokenAtual == VARIAVEL)
        compila_var();

    // reserva memoria para as variaveis locais
    if (numLocais[nivel] > 0)
        gera("AMEM %d", numLocais[nivel]);

    // o corpo (BEGIN..END) vem depois das rotinas aninhadas:
    // e preciso pular o codigo delas na execucao
    if (tokenAtual == PROCEDURE || tokenAtual == FUNCTION) {

        rotPula = novoRotulo();
        gera("DSVS L%d", rotPula);

        while (
            tokenAtual == PROCEDURE ||
            tokenAtual == FUNCTION
        ) {

            if (tokenAtual == PROCEDURE)
                compila_procedure();
            else
                compila_function();
        }

        geraRot(rotPula, "NADA");
    }

    if (tokenAtual != BEGIN_TOKEN)
        erroSintatico("BEGIN", "esperava BEGIN");

    compila_comando();

    // libera as variaveis locais
    if (numLocais[nivel] > 0)
        gera("DMEM %d", numLocais[nivel]);
}

// ================= PROGRAMA =================
void compila_programa() {

    TRACE("Reconhecendo PROGRAM\n");

    consome(PROGRAMA, "esperava PROGRAM");

    declara(NAT_PROGRAMA, IDENTIFICADOR, "esperava identificador");

    if (tokenAtual == ABRE_PARENTESES) {

        TRACE("Reconhecendo parametros de entrada e saida\n");

        avancaToken();

        declara(NAT_PARAM_PROGRAMA, IDENTIFICADOR, "esperava identificador");

        while (tokenAtual == VIRGULA) {

            avancaToken();

            declara(NAT_PARAM_PROGRAMA, IDENTIFICADOR, "esperava identificador");
        }

        consome(FECHA_PARENTESES, "esperava )");
    }

    consome(PONTO_E_VIRGULA, "esperava ;");

    entraEscopo();

    rotinaDoNivel[nivelAtual()] = -1;

    gera("INPP");

    compila_bloco();

    gera("PARA");

    saiEscopo();
    saiEscopo();

    consome(PONTO, "esperava .");

    consome(FIM_DE_ARQUIVO, "esperava fim de arquivo");

    printf("\n========== ANALISE SINTATICA FINALIZADA ==========\n");
    printf("Status: SUCESSO\n");
    printf("Programa sintaticamente e semanticamente correto.\n");
    printf("Linhas analisadas: %d\n", linha);
    printf("Codigo MEPA gerado em: %s\n", nomeSaida);
    printf("==================================================\n");
}

// ================= SIMBOLOS PRE-DEFINIDOS =================
void iniciaTabela() {

    int idx;

    escopoAtual = 0;

    insere("integer", NAT_TIPO, TIPO_INTEGER, 0, 0);

    idx = insere("read", NAT_PROCEDURE, TIPO_NULO, 0, 0);
    tabela[idx].numParams = -1;
    tabela[idx].especial = ESP_READ;

    idx = insere("write", NAT_PROCEDURE, TIPO_NULO, 0, 0);
    tabela[idx].numParams = -1;
    tabela[idx].especial = ESP_WRITE;
}

// ================= MAIN =================
int main(int argc, char *argv[]) {

    char *arquivo = "entrada.txt";

    if (argc > 1)
        arquivo = argv[1];

    if (argc > 2)
        nomeSaida = argv[2];

    fp = fopen(arquivo, "r");

    if (fp == NULL) {

        printf("Erro ao abrir arquivo %s\n", arquivo);
        return 1;
    }

    saida = fopen(nomeSaida, "w");

    if (saida == NULL) {

        printf("Erro ao criar arquivo de saida %s\n", nomeSaida);
        fclose(fp);
        return 1;
    }

    iniciaTabela();

    avancaToken();

    compila_programa();

    fclose(fp);
    fclose(saida);

    // Mostra o codigo MEPA gerado tambem na tela (util em compiladores
    // online, onde o arquivo criado nem sempre aparece na lista de arquivos)
    if (MOSTRAR_SAIDA_NA_TELA) {

        FILE *leitura = fopen(nomeSaida, "r");
        int ch;

        if (leitura != NULL) {

            printf("\n========== CODIGO MEPA GERADO ==========\n");

            while ((ch = fgetc(leitura)) != EOF)
                putchar(ch);

            printf("========================================\n");

            fclose(leitura);
        }
    }

    return 0;
}