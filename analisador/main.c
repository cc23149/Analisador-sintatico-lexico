/******************************************************************************
 ANALISADOR LEXICO + SINTATICO + TABELA DE SIMBOLOS  (Pascal simplificado)

 Compilar: gcc -Wall -o main main.c
 Executar: ./main                (le entrada.txt)
           ./main outro.txt      (le o arquivo indicado)
*******************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// Coloque 1 para ver o passo a passo "Reconhecendo ..." (bom para apresentar)
#define TRACE_ON 0
#define TRACE(...) do { if (TRACE_ON) printf(__VA_ARGS__); } while (0)

#define MAX_ID       64
#define MAX_SIMBOLOS 500

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
FILE *fp;
Token tokenAtual;

// texto (lexema) do tokenAtual - so vale para IDENTIFICADOR e NUMERO
char lexema[MAX_ID];

// ================= CONTROLE DE POSICAO =================
int linha = 1;      // posicao de leitura do arquivo (fica na frente do token)
int coluna = 0;

int tokenLinha = 1; // posicao onde o tokenAtual COMECA (usada nas mensagens de erro)
int tokenColuna = 0;

// ================= TABELA DE SIMBOLOS =================
//
// Cada simbolo guarda: identificador, tipo, natureza e escopo.
//
// Escopo (camadas):
//   0 = onde o programa e declarado (program, input, output, read, write, integer)
//   1 = bloco principal do programa
//   2 = dentro de um procedure/function declarado no bloco principal
//   3 = dentro de um procedure/function declarado dentro de outro ... e assim por diante
//
// A tabela funciona como uma PILHA: ao entrar num procedure/function o escopo
// aumenta; ao sair, os simbolos daquele escopo sao removidos.

typedef enum {
    TIPO_NULO,          // nao tem tipo (program, procedure, label...)
    TIPO_INTEGER
} TipoBase;

typedef enum {
    NAT_PROGRAMA,
    NAT_PARAM_PROGRAMA, // input, output
    NAT_TIPO,           // integer
    NAT_VARIAVEL,
    NAT_PARAMETRO,      // parametro por valor
    NAT_PARAMETRO_REF,  // parametro por referencia (var)
    NAT_PROCEDURE,
    NAT_FUNCTION,
    NAT_LABEL
} Natureza;

typedef struct {
    char     identificador[MAX_ID];
    TipoBase tipo;
    Natureza natureza;
    int      escopo;
} Simbolo;

Simbolo tabela[MAX_SIMBOLOS];
int topo = 0;          // proxima posicao livre da tabela
int escopoAtual = 0;

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

// ================= ERROS =================
void erroLexico(char mensagem[], int c) {

    printf("\n========== ERRO LEXICO ==========\n");
    printf("Linha: %d\n", linha);
    printf("Coluna: %d\n", coluna);

    if (c > 0)
        printf("Caractere encontrado: %c\n", c);

    printf("Descricao: %s\n", mensagem);
    printf("=================================\n");

    exit(1);
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

    exit(1);
}

void erroSemantico(char mensagem[], char nome[], int l, int c) {

    printf("\n========== ERRO SEMANTICO ==========\n");
    printf("Linha: %d\n", l);
    printf("Coluna: %d\n", c);
    printf("Identificador: %s\n", nome);
    printf("Escopo atual: %d\n", escopoAtual);
    printf("Descricao: %s\n", mensagem);
    printf("====================================\n");

    exit(1);
}

// ================= ANALISADOR LEXICO =================
Token analex() {

    int c;              // int (e nao char) para conseguir comparar com EOF
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

        // aqui comeca um token (ou comentario): guarda a posicao
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

            buffer[i++] = tolower(c);   // Pascal nao diferencia maiuscula/minuscula

            while (isalnum(c = fgetc(fp)) || c == '_') {

                if (i >= MAX_ID - 1)
                    erroLexico("identificador muito grande", 0);

                buffer[i++] = tolower(c);
                coluna++;
            }

            buffer[i] = '\0';

            ungetc(c, fp);

            strcpy(lexema, buffer);     // sintatico/tabela usam o lexema

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

// procura o simbolo do escopo mais interno para o mais externo
// (como a tabela e uma pilha, basta percorrer de tras para frente)
int busca(char nome[]) {

    int i;

    for (i = topo - 1; i >= 0; i--)
        if (strcmp(tabela[i].identificador, nome) == 0)
            return i;

    return -1;
}

// insere no escopo atual; nao permite repetir o nome no MESMO escopo
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

    topo++;

    return topo - 1;
}

// usado em "a, b, c : integer": os nomes entram sem tipo e o tipo
// so e conhecido depois do ':'
void defineTipo(int inicio, TipoBase tipo) {

    int i;

    for (i = inicio; i < topo; i++)
        tabela[i].tipo = tipo;
}

void imprimeEscopo(int escopo) {

    int i;

    printf("\n---------- TABELA DE SIMBOLOS - ESCOPO %d ----------\n", escopo);
    printf("%-16s %-16s %-9s\n", "IDENTIFICADOR", "NATUREZA", "TIPO");

    for (i = 0; i < topo; i++)
        if (tabela[i].escopo == escopo)
            printf("%-16s %-16s %-9s\n",
                   tabela[i].identificador,
                   nomeNatureza(tabela[i].natureza),
                   nomeTipo(tabela[i].tipo));
}

void entraEscopo() {

    escopoAtual++;
}

// mostra a camada que esta terminando e remove seus simbolos
void saiEscopo() {

    imprimeEscopo(escopoAtual);

    while (topo > 0 && tabela[topo - 1].escopo == escopoAtual)
        topo--;

    escopoAtual--;
}

// declara o identificador (ou numero, no caso de label) que esta em tokenAtual
int declara(Natureza nat, Token esperado, char mensagem[]) {

    char nome[MAX_ID];
    int l = tokenLinha, c = tokenColuna;

    strcpy(nome, lexema);           // copia ANTES de avancar (lexema muda)

    consome(esperado, mensagem);

    return insere(nome, nat, TIPO_NULO, l, c);
}

// declara um nome dentro de uma lista "a, b, c : tipo" (var e parametros).
// Depois do nome so pode vir ',' ou ':'. Conferir isso ANTES de inserir evita
// um erro semantico enganoso quando o problema real e sintatico
// (ex.: faltou BEGIN e o "x := 10" foi lido como se fosse uma declaracao).
int declaraNaLista(Natureza nat) {

    char nome[MAX_ID];
    int l = tokenLinha, c = tokenColuna;

    strcpy(nome, lexema);

    consome(IDENTIFICADOR, "esperava identificador");

    if (tokenAtual != VIRGULA && tokenAtual != DOIS_PONTOS)
        erroSintatico(":", "esperava : na declaracao (faltou BEGIN?)");

    return insere(nome, nat, TIPO_NULO, l, c);
}

// tokenAtual e um IDENTIFICADOR que deve ja estar declarado.
// Consome o token e devolve a posicao na tabela.
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

// tokenAtual e um NUMERO que deve ser um label declarado
void usaLabel() {

    char nome[MAX_ID];
    int l = tokenLinha, c = tokenColuna;
    int idx;

    strcpy(nome, lexema);

    consome(NUMERO, "esperava numero");

    idx = busca(nome);

    if (idx < 0 || tabela[idx].natureza != NAT_LABEL)
        erroSemantico("label nao declarado", nome, l, c);
}

// le um tipo (ex.: integer) e procura na tabela
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

// ================= DECLARACOES =================
void compila_expressao();
void compila_comando();
void compila_bloco();

// ================= CHAMADA (argumentos entre parenteses) =================
// tokenAtual == '('
void compila_argumentos() {

    consome(ABRE_PARENTESES, "esperava (");

    if (tokenAtual != FECHA_PARENTESES) {

        compila_expressao();

        while (tokenAtual == VIRGULA) {

            avancaToken();
            compila_expressao();
        }
    }

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

            compila_argumentos();
        }
        else if (nat != NAT_VARIAVEL && nat != NAT_PARAMETRO &&
                 nat != NAT_PARAMETRO_REF && nat != NAT_FUNCTION) {

            erroSemantico("identificador nao pode ser usado em expressao", nome, l, c);
        }
    }

    else if (tokenAtual == NUMERO) {

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

    TRACE("Reconhecendo TERMO\n");

    compila_fator();

    while (tokenAtual == VEZES || tokenAtual == DIVIDIR) {

        avancaToken();
        compila_fator();
    }
}

// ================= EXPRESSAO SIMPLES =================
void compila_expressao_simples() {

    TRACE("Reconhecendo EXPRESSAO SIMPLES\n");

    // sinal opcional no inicio: -x  ou  +x
    if (tokenAtual == MAIS || tokenAtual == MENOS)
        avancaToken();

    compila_termo();

    while (tokenAtual == MAIS || tokenAtual == MENOS) {

        avancaToken();
        compila_termo();
    }
}

// ================= EXPRESSAO =================
void compila_expressao() {

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

        avancaToken();

        compila_expressao_simples();
    }
}

// ================= PARAMETROS =================
// um grupo:  [var] a, b : tipo
void compila_grupo_parametros() {

    Natureza nat = NAT_PARAMETRO;
    int inicio;
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
}

void compila_parametros() {

    TRACE("Reconhecendo PARAMETROS\n");

    consome(ABRE_PARENTESES, "esperava (");

    if (tokenAtual != FECHA_PARENTESES) {

        compila_grupo_parametros();

        // grupos sao separados por ';'
        while (tokenAtual == PONTO_E_VIRGULA) {

            avancaToken();
            compila_grupo_parametros();
        }
    }

    consome(FECHA_PARENTESES, "esperava ) ou ;");
}

// ================= LABEL =================
void compila_label() {

    TRACE("Reconhecendo LABEL\n");

    consome(LABEL, "esperava LABEL");

    declara(NAT_LABEL, NUMERO, "esperava numero");

    while (tokenAtual == VIRGULA) {

        avancaToken();
        declara(NAT_LABEL, NUMERO, "esperava numero");
    }

    consome(PONTO_E_VIRGULA, "esperava ;");
}

// ================= VAR =================
void compila_var() {

    TRACE("Reconhecendo declaracao VAR\n");

    consome(VARIAVEL, "esperava VAR");

    while (tokenAtual == IDENTIFICADOR) {

        int inicio = topo;
        TipoBase tipo;

        declaraNaLista(NAT_VARIAVEL);

        while (tokenAtual == VIRGULA) {

            avancaToken();
            declaraNaLista(NAT_VARIAVEL);
        }

        consome(DOIS_PONTOS, "esperava :");

        tipo = compila_tipo();

        defineTipo(inicio, tipo);

        consome(PONTO_E_VIRGULA, "esperava ;");
    }
}

// ================= PROCEDURE =================
void compila_procedure() {

    TRACE("Reconhecendo PROCEDURE\n");

    consome(PROCEDURE, "esperava PROCEDURE");

    // o nome pertence ao escopo de FORA...
    declara(NAT_PROCEDURE, IDENTIFICADOR, "esperava identificador");

    // ...parametros e corpo pertencem a uma camada nova
    entraEscopo();

    if (tokenAtual == ABRE_PARENTESES)
        compila_parametros();

    consome(PONTO_E_VIRGULA, "esperava ;");

    compila_bloco();

    saiEscopo();

    consome(PONTO_E_VIRGULA, "esperava ;");
}

// ================= FUNCTION =================
void compila_function() {

    int idx;

    TRACE("Reconhecendo FUNCTION\n");

    consome(FUNCTION, "esperava FUNCTION");

    idx = declara(NAT_FUNCTION, IDENTIFICADOR, "esperava identificador");

    entraEscopo();

    if (tokenAtual == ABRE_PARENTESES)
        compila_parametros();

    consome(DOIS_PONTOS, "esperava :");

    // o tipo da funcao e o tipo de retorno
    tabela[idx].tipo = compila_tipo();

    consome(PONTO_E_VIRGULA, "esperava ;");

    compila_bloco();

    saiEscopo();

    consome(PONTO_E_VIRGULA, "esperava ;");
}

// ================= COMANDO =================
void compila_comando() {

    TRACE("Reconhecendo COMANDO\n");

    if (tokenAtual == NUMERO) {

        TRACE("Reconhecendo LABEL de comando\n");

        usaLabel();

        consome(DOIS_PONTOS, "esperava :");

        compila_comando();
    }

    else if (tokenAtual == BEGIN_TOKEN) {

        TRACE("Reconhecendo bloco BEGIN END\n");

        avancaToken();

        compila_comando();

        // comandos separados por ';'
        while (tokenAtual == PONTO_E_VIRGULA) {

            avancaToken();
            compila_comando();
        }

        consome(END_TOKEN, "esperava END (ou ; entre comandos)");
    }

    else if (tokenAtual == IF_TOKEN) {

        TRACE("Reconhecendo comando IF\n");

        avancaToken();

        compila_expressao();

        consome(THEN_TOKEN, "esperava THEN");

        compila_comando();

        if (tokenAtual == ELSE_TOKEN) {

            TRACE("Reconhecendo ELSE\n");

            avancaToken();

            compila_comando();
        }
    }

    else if (tokenAtual == GOTO) {

        TRACE("Reconhecendo comando GOTO\n");

        avancaToken();

        usaLabel();
    }

    else if (tokenAtual == IDENTIFICADOR) {

        char nome[MAX_ID];
        int l = tokenLinha, c = tokenColuna;
        int idx = usaIdentificador();
        Natureza nat = tabela[idx].natureza;

        strcpy(nome, tabela[idx].identificador);

        if (tokenAtual == ATRIBUICAO) {

            TRACE("Reconhecendo atribuicao\n");

            // so recebe valor: variavel, parametro ou nome da funcao (retorno)
            if (nat != NAT_VARIAVEL && nat != NAT_PARAMETRO &&
                nat != NAT_PARAMETRO_REF && nat != NAT_FUNCTION)
                erroSemantico("identificador nao pode receber atribuicao", nome, l, c);

            avancaToken();

            compila_expressao();
        }

        else if (tokenAtual == ABRE_PARENTESES) {

            TRACE("Reconhecendo chamada de procedimento\n");

            if (nat != NAT_PROCEDURE)
                erroSemantico("so procedure pode ser chamada como comando", nome, l, c);

            compila_argumentos();
        }

        else {

            // identificador sozinho so faz sentido como chamada de procedure (ex.: q)
            if (nat != NAT_PROCEDURE)
                erroSintatico(":=", "esperava := ou chamada de procedure");
        }
    }

    // comando vazio (ex.: "x := 1;" logo antes do END)
    else if (tokenAtual == PONTO_E_VIRGULA ||
             tokenAtual == END_TOKEN ||
             tokenAtual == ELSE_TOKEN) {

        // nada a fazer
    }

    else {

        erroSintatico(NULL, "comando invalido");
    }
}

// ================= BLOCO =================
void compila_bloco() {

    TRACE("Reconhecendo BLOCO\n");

    if (tokenAtual == LABEL)
        compila_label();

    if (tokenAtual == VARIAVEL)
        compila_var();

    while (
        tokenAtual == PROCEDURE ||
        tokenAtual == FUNCTION
    ) {

        if (tokenAtual == PROCEDURE)
            compila_procedure();
        else
            compila_function();
    }

    // o corpo (BEGIN ... END) e um comando composto
    if (tokenAtual != BEGIN_TOKEN)
        erroSintatico("BEGIN", "esperava BEGIN");

    compila_comando();
}

// ================= PROGRAMA =================
void compila_programa() {

    TRACE("Reconhecendo PROGRAM\n");

    consome(PROGRAMA, "esperava PROGRAM");

    // escopo 0: o programa e seus parametros
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

    // escopo 1: bloco principal
    entraEscopo();

    compila_bloco();

    saiEscopo();    // mostra escopo 1
    saiEscopo();    // mostra escopo 0

    consome(PONTO, "esperava .");

    consome(FIM_DE_ARQUIVO, "esperava fim de arquivo");

    printf("\n========== ANALISE SINTATICA FINALIZADA ==========\n");
    printf("Status: SUCESSO\n");
    printf("Programa sintaticamente e semanticamente correto.\n");
    printf("Linhas analisadas: %d\n", linha);
    printf("==================================================\n");
}

// ================= SIMBOLOS PRE-DEFINIDOS (escopo 0) =================
void iniciaTabela() {

    escopoAtual = 0;

    insere("integer", NAT_TIPO, TIPO_INTEGER, 0, 0);
    insere("read",    NAT_PROCEDURE, TIPO_NULO, 0, 0);
    insere("write",   NAT_PROCEDURE, TIPO_NULO, 0, 0);
}

// ================= MAIN =================
int main(int argc, char *argv[]) {

    char *arquivo = "entrada.txt";

    if (argc > 1)
        arquivo = argv[1];

    fp = fopen(arquivo, "r");

    if (fp == NULL) {

        printf("Erro ao abrir arquivo %s\n", arquivo);
        return 1;
    }

    iniciaTabela();

    avancaToken();

    compila_programa();

    fclose(fp);

    return 0;
}
