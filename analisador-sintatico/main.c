/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

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

// ================= CONTROLE DE POSICAO =================
int linha = 1;
int coluna = 0;

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

// ================= ANALISADOR LEXICO =================
Token analex() {

    char c;
    char buffer[100];
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

        // ================= COMENTARIOS =================
        if (c == '(') {

            char prox = fgetc(fp);
            coluna++;

            if (prox == '*') {

                char ant = 0;
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

                if (!fechou) {

                    printf("\n========== ERRO LEXICO ==========\n");
                    printf("Linha: %d\n", linha);
                    printf("Coluna: %d\n", coluna);
                    printf("Descricao: comentario nao fechado\n");
                    printf("=================================\n");

                    exit(1);
                }

                continue;
            }

            ungetc(prox, fp);
            coluna--;

            return ABRE_PARENTESES;
        }

        // ================= IDENTIFICADORES =================
        if (isalpha(c)) {

            i = 0;

            buffer[i++] = c;

            while (isalnum(c = fgetc(fp)) || c == '_') {

                buffer[i++] = c;
                coluna++;
            }

            buffer[i] = '\0';

            ungetc(c, fp);

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

            while (isdigit(c = fgetc(fp)))
                coluna++;

            ungetc(c, fp);

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
        printf("\n========== ERRO LEXICO ==========\n");
        printf("Linha: %d\n", linha);
        printf("Coluna: %d\n", coluna);
        printf("Caractere invalido: %c\n", c);
        printf("=================================\n");

        exit(1);
    }

    return FIM_DE_ARQUIVO;
}

// ================= AVANCA TOKEN =================
void avancaToken() {

    tokenAtual = analex();
}

// ================= CONSOME TOKEN =================
void consome(Token esperado, char mensagem[]) {

    if (tokenAtual == esperado) {

        avancaToken();
    }
    else {

        printf("\n========== ERRO SINTATICO ==========\n");

        printf("Linha: %d\n", linha);
        printf("Coluna: %d\n", coluna);

        printf(
            "Token encontrado: %s\n",
            nomeToken(tokenAtual)
        );

        printf(
            "Token esperado: %s\n",
            nomeToken(esperado)
        );

        printf("Descricao: %s\n", mensagem);

        printf("====================================\n");

        exit(1);
    }
}

// ================= DECLARACOES =================
void compila_expressao();
void compila_comando();
void compila_bloco();

// ================= FATOR =================
void compila_fator() {

    printf("Reconhecendo FATOR\n");

    if (tokenAtual == IDENTIFICADOR) {

        avancaToken();

        if (tokenAtual == ABRE_PARENTESES) {

            printf("Reconhecendo chamada de funcao\n");

            avancaToken();

            if (tokenAtual != FECHA_PARENTESES) {

                compila_expressao();

                while (tokenAtual == VIRGULA) {

                    avancaToken();
                    compila_expressao();
                }
            }

            consome(FECHA_PARENTESES, "esperava )");
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

        printf("\n========== ERRO SINTATICO ==========\n");

        printf("Linha: %d\n", linha);
        printf("Coluna: %d\n", coluna);

        printf(
            "Token encontrado: %s\n",
            nomeToken(tokenAtual)
        );

        printf("Descricao: fator invalido\n");

        printf("====================================\n");

        exit(1);
    }
}

// ================= TERMO =================
void compila_termo() {

    printf("Reconhecendo TERMO\n");

    compila_fator();

    while (tokenAtual == VEZES || tokenAtual == DIVIDIR) {

        avancaToken();
        compila_fator();
    }
}

// ================= EXPRESSAO SIMPLES =================
void compila_expressao_simples() {

    printf("Reconhecendo EXPRESSAO SIMPLES\n");

    compila_termo();

    while (tokenAtual == MAIS || tokenAtual == MENOS) {

        avancaToken();
        compila_termo();
    }
}

// ================= EXPRESSAO =================
void compila_expressao() {

    printf("Reconhecendo EXPRESSAO\n");

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
void compila_parametros() {

    printf("Reconhecendo PARAMETROS\n");

    consome(ABRE_PARENTESES, "esperava (");

    while (tokenAtual != FECHA_PARENTESES) {

        if (tokenAtual == VARIAVEL)
            avancaToken();

        consome(IDENTIFICADOR, "esperava identificador");

        while (tokenAtual == VIRGULA) {

            avancaToken();
            consome(IDENTIFICADOR, "esperava identificador");
        }

        consome(DOIS_PONTOS, "esperava :");

        consome(IDENTIFICADOR, "esperava tipo");

        if (tokenAtual == PONTO_E_VIRGULA)
            avancaToken();
    }

    consome(FECHA_PARENTESES, "esperava )");
}

// ================= LABEL =================
void compila_label() {

    printf("Reconhecendo LABEL\n");

    consome(LABEL, "esperava LABEL");

    consome(NUMERO, "esperava numero");

    while (tokenAtual == VIRGULA) {

        avancaToken();
        consome(NUMERO, "esperava numero");
    }

    consome(PONTO_E_VIRGULA, "esperava ;");
}

// ================= VAR =================
void compila_var() {

    printf("Reconhecendo declaracao VAR\n");

    consome(VARIAVEL, "esperava VAR");

    while (tokenAtual == IDENTIFICADOR) {

        consome(IDENTIFICADOR, "esperava identificador");

        while (tokenAtual == VIRGULA) {

            avancaToken();
            consome(IDENTIFICADOR, "esperava identificador");
        }

        consome(DOIS_PONTOS, "esperava :");

        consome(IDENTIFICADOR, "esperava tipo");

        consome(PONTO_E_VIRGULA, "esperava ;");
    }
}

// ================= PROCEDURE =================
void compila_procedure() {

    printf("Reconhecendo PROCEDURE\n");

    consome(PROCEDURE, "esperava PROCEDURE");

    consome(IDENTIFICADOR, "esperava identificador");

    if (tokenAtual == ABRE_PARENTESES)
        compila_parametros();

    consome(PONTO_E_VIRGULA, "esperava ;");

    compila_bloco();

    consome(PONTO_E_VIRGULA, "esperava ;");
}

// ================= FUNCTION =================
void compila_function() {

    printf("Reconhecendo FUNCTION\n");

    consome(FUNCTION, "esperava FUNCTION");

    consome(IDENTIFICADOR, "esperava identificador");

    if (tokenAtual == ABRE_PARENTESES)
        compila_parametros();

    consome(DOIS_PONTOS, "esperava :");

    consome(IDENTIFICADOR, "esperava tipo");

    consome(PONTO_E_VIRGULA, "esperava ;");

    compila_bloco();

    consome(PONTO_E_VIRGULA, "esperava ;");
}

// ================= COMANDO =================
void compila_comando() {

    printf("Reconhecendo COMANDO\n");

    if (tokenAtual == NUMERO) {

        printf("Reconhecendo LABEL de comando\n");

        avancaToken();

        consome(DOIS_PONTOS, "esperava :");

        compila_comando();
    }

    else if (tokenAtual == BEGIN_TOKEN) {

        printf("Reconhecendo bloco BEGIN END\n");

        avancaToken();

        while (tokenAtual != END_TOKEN) {

            compila_comando();

            if (tokenAtual == PONTO_E_VIRGULA)
                avancaToken();
        }

        consome(END_TOKEN, "esperava END");
    }

    else if (tokenAtual == IF_TOKEN) {

        printf("Reconhecendo comando IF\n");

        avancaToken();

        compila_expressao();

        consome(THEN_TOKEN, "esperava THEN");

        compila_comando();

        if (tokenAtual == ELSE_TOKEN) {

            printf("Reconhecendo ELSE\n");

            avancaToken();

            compila_comando();
        }
    }

    else if (tokenAtual == GOTO) {

        printf("Reconhecendo comando GOTO\n");

        avancaToken();

        consome(NUMERO, "esperava label");
    }

    else if (tokenAtual == IDENTIFICADOR) {

        printf("Reconhecendo identificador\n");

        avancaToken();

        if (tokenAtual == ATRIBUICAO) {

            printf("Reconhecendo atribuicao\n");

            avancaToken();

            compila_expressao();
        }

        else if (tokenAtual == ABRE_PARENTESES) {

            printf("Reconhecendo chamada de procedimento\n");

            avancaToken();

            if (tokenAtual != FECHA_PARENTESES) {

                compila_expressao();

                while (tokenAtual == VIRGULA) {

                    avancaToken();
                    compila_expressao();
                }
            }

            consome(FECHA_PARENTESES, "esperava )");
        }
    }
}

// ================= BLOCO =================
void compila_bloco() {

    printf("Reconhecendo BLOCO\n");

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

    consome(BEGIN_TOKEN, "esperava BEGIN");

    while (tokenAtual != END_TOKEN) {

        compila_comando();

        if (tokenAtual == PONTO_E_VIRGULA)
            avancaToken();
    }

    consome(END_TOKEN, "esperava END");
}

// ================= PROGRAMA =================
void compila_programa() {

    printf("Reconhecendo PROGRAM\n");

    consome(PROGRAMA, "esperava PROGRAM");

    consome(IDENTIFICADOR, "esperava identificador");

    if (tokenAtual == ABRE_PARENTESES) {

        printf("Reconhecendo parametros de entrada e saida\n");

        avancaToken();

        consome(IDENTIFICADOR, "esperava identificador");

        while (tokenAtual == VIRGULA) {

            avancaToken();

            consome(IDENTIFICADOR, "esperava identificador");
        }

        consome(FECHA_PARENTESES, "esperava )");
    }

    consome(PONTO_E_VIRGULA, "esperava ;");

    compila_bloco();

    consome(PONTO, "esperava .");

    consome(FIM_DE_ARQUIVO, "esperava fim de arquivo");

    printf("\n========== ANALISE SINTATICA FINALIZADA ==========\n");
    printf("Status: SUCESSO\n");
    printf("Programa sintaticamente correto.\n");
    printf("Linhas analisadas: %d\n", linha);
    printf("==================================================\n");
}

// ================= MAIN =================
int main() {

    fp = fopen("entrada.txt", "r");

    if (fp == NULL) {

        printf("Erro ao abrir arquivo\n");
        return 1;
    }

    avancaToken();

    compila_programa();

    fclose(fp);

    return 0;
}