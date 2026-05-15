// ================= MAIN.C =================

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

// ================= ANALISADOR LÉXICO =================
Token analex() {

    char c;
    char buffer[100];
    int i;

    while ((c = fgetc(fp)) != EOF) {

        // IGNORA ESPAÇOS
        if (isspace(c))
            continue;

        // IGNORA COMENTÁRIOS (* *)
        if (c == '(') {

            char prox = fgetc(fp);

            if (prox == '*') {

                char ant = 0;

                while ((c = fgetc(fp)) != EOF) {

                    if (ant == '*' && c == ')')
                        break;

                    ant = c;
                }

                continue;
            }

            ungetc(prox, fp);
            return ABRE_PARENTESES;
        }

        // IDENTIFICADOR OU PALAVRA RESERVADA
        if (isalpha(c)) {

            i = 0;
            buffer[i++] = c;

            while (isalnum(c = fgetc(fp)) || c == '_') {
                buffer[i++] = c;
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

        // NÚMEROS
        else if (isdigit(c)) {

            while (isdigit(c = fgetc(fp)));

            ungetc(c, fp);

            return NUMERO;
        }

        // OPERADORES
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

                    if (c == '=')
                        return MAIOROUIGUAL;

                    ungetc(c, fp);
                    return MAIOR;

                case '<':
                    c = fgetc(fp);

                    if (c == '=')
                        return MENOROUIGUAL;

                    else if (c == '>')
                        return DIFERENTE;

                    ungetc(c, fp);
                    return MENOR;

                case ':':
                    c = fgetc(fp);

                    if (c == '=')
                        return ATRIBUICAO;

                    ungetc(c, fp);
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

        return DESCONHECIDO;
    }

    return FIM_DE_ARQUIVO;
}

// ================= AVANÇA TOKEN =================
void avancaToken() {
    tokenAtual = analex();
}

// ================= CONSOME TOKEN =================
void consome(Token esperado, char mensagem[]) {

    if (tokenAtual == esperado) {
        avancaToken();
    }
    else {
        printf("Erro sintatico: %s\n", mensagem);
        exit(1);
    }
}

// ================= DECLARAÇÕES =================
void compila_expressao();
void compila_comando();
void compila_bloco();

// ================= FATOR =================
void compila_fator() {

    if (tokenAtual == IDENTIFICADOR) {

        avancaToken();

        // CHAMADA DE FUNÇÃO
        if (tokenAtual == ABRE_PARENTESES) {

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

        printf("Erro sintatico: fator invalido\n");
        exit(1);
    }
}

// ================= TERMO =================
void compila_termo() {

    compila_fator();

    while (tokenAtual == VEZES || tokenAtual == DIVIDIR) {

        avancaToken();
        compila_fator();
    }
}

// ================= EXPRESSÃO SIMPLES =================
void compila_expressao_simples() {

    compila_termo();

    while (tokenAtual == MAIS || tokenAtual == MENOS) {

        avancaToken();
        compila_termo();
    }
}

// ================= EXPRESSÃO =================
void compila_expressao() {

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

// ================= PARÂMETROS =================
void compila_parametros() {

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

    // LABEL
    if (tokenAtual == NUMERO) {

        avancaToken();

        consome(DOIS_PONTOS, "esperava :");

        compila_comando();
    }

    // BEGIN END
    else if (tokenAtual == BEGIN_TOKEN) {

        avancaToken();

        while (tokenAtual != END_TOKEN) {

            compila_comando();

            if (tokenAtual == PONTO_E_VIRGULA)
                avancaToken();
        }

        consome(END_TOKEN, "esperava END");
    }

    // IF
    else if (tokenAtual == IF_TOKEN) {

        avancaToken();

        compila_expressao();

        consome(THEN_TOKEN, "esperava THEN");

        compila_comando();

        if (tokenAtual == ELSE_TOKEN) {

            avancaToken();

            compila_comando();
        }
    }

    // GOTO
    else if (tokenAtual == GOTO) {

        avancaToken();

        consome(NUMERO, "esperava label");
    }

    // IDENTIFICADOR
    else if (tokenAtual == IDENTIFICADOR) {

        avancaToken();

        // ATRIBUIÇÃO
        if (tokenAtual == ATRIBUICAO) {

            avancaToken();

            compila_expressao();
        }

        // CHAMADA
        else if (tokenAtual == ABRE_PARENTESES) {

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

    consome(PROGRAMA, "esperava PROGRAM");

    consome(IDENTIFICADOR, "esperava identificador");

    // (input,output)
    if (tokenAtual == ABRE_PARENTESES) {

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

    printf("Programa sintaticamente correto!\n");
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
