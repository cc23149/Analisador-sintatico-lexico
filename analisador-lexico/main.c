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

// ================= CONTROLE DE POSIÇÃO =================
int linha = 1;
int coluna = 0;

// guarda o último lexema encontrado
char lexemaAtual[100];

// ================= FUNÇÃO DE ERRO LÉXICO =================
void erroLexico(char mensagem[], char c) {

    printf("\n========== ERRO LEXICO ==========\n");

    printf("Linha: %d\n", linha);
    printf("Coluna: %d\n", coluna);

    if (c != '\0')
        printf("Caractere encontrado: %c\n", c);

    printf("Descricao: %s\n", mensagem);

    exit(1);
}

// ================= IDENTIFICA PALAVRAS RESERVADAS =================
void palavraReservada(char *str) {

    if (strcmp(str, "program") == 0)
        printf("programa\n");

    else if (strcmp(str, "label") == 0)
        printf("rotulo\n");

    else if (strcmp(str, "type") == 0)
        printf("tipo\n");

    else if (strcmp(str, "var") == 0)
        printf("variavel\n");

    else if (strcmp(str, "procedure") == 0)
        printf("procedimento\n");

    else if (strcmp(str, "function") == 0)
        printf("funcao\n");

    else if (strcmp(str, "begin") == 0)
        printf("inicio\n");

    else if (strcmp(str, "end") == 0)
        printf("fim\n");

    else if (strcmp(str, "if") == 0)
        printf("se\n");

    else if (strcmp(str, "then") == 0)
        printf("entao\n");

    else if (strcmp(str, "else") == 0)
        printf("senao\n");

    else if (strcmp(str, "while") == 0)
        printf("enquanto\n");

    else if (strcmp(str, "do") == 0)
        printf("faca\n");

    else if (strcmp(str, "goto") == 0)
        printf("vapara\n");

    else if (strcmp(str, "and") == 0)
        printf("e\n");

    else if (strcmp(str, "or") == 0)
        printf("ou\n");

    else if (strcmp(str, "not") == 0)
        printf("nao\n");

    else if (strcmp(str, "div") == 0)
        printf("dividir\n");

    else
        printf("identificador\n");
}

// ================= MAIN =================
int main() {

    FILE *fp = fopen("entrada.txt", "r");

    char c;
    char buffer[100];
    int i;

    // ================= ABERTURA DE ARQUIVO =================
    if (fp == NULL) {

        printf("Erro ao abrir arquivo\n");
        return 1;
    }

    // ================= LEITURA PRINCIPAL =================
    while (!feof(fp)) {

        c = fgetc(fp);

        if (feof(fp))
            break;

        coluna++;

        // ================= CONTROLE DE LINHA =================
        if (c == '\n') {

            linha++;
            coluna = 0;
            continue;
        }

        // ================= IGNORA ESPAÇOS =================
        if (isspace(c))
            continue;

        // ================= COMENTÁRIOS =================
        if (c == '(') {

            char prox = fgetc(fp);
            coluna++;

            // comentário Pascal
            if (prox == '*') {

                char ant = 0;
                int fechou = 0;

                while (!feof(fp)) {

                    c = fgetc(fp);

                    if (feof(fp))
                        break;

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

                // comentário não fechado
                if (!fechou) {

                    erroLexico(
                        "comentario iniciado com (* nao foi fechado",
                        '\0'
                    );
                }

                continue;
            }

            // não era comentário
            ungetc(prox, fp);
            coluna--;

            printf("abreparenteses\n");
            continue;
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

            strcpy(lexemaAtual, buffer);

            palavraReservada(buffer);

            ungetc(c, fp);
        }

        // ================= NÚMEROS =================
        else if (isdigit(c)) {

            i = 0;

            buffer[i++] = c;

            while (isdigit(c = fgetc(fp))) {

                buffer[i++] = c;
                coluna++;
            }

            buffer[i] = '\0';

            strcpy(lexemaAtual, buffer);

            printf("numero\n");

            ungetc(c, fp);
        }

        // ================= OPERADORES E SÍMBOLOS =================
        else {

            switch (c) {

                case '+':
                    printf("mais\n");
                    break;

                case '-':
                    printf("menos\n");
                    break;

                case '*':
                    printf("vezes\n");
                    break;

                case '=':
                    printf("igual\n");
                    break;

                case '<':

                    c = fgetc(fp);
                    coluna++;

                    if (c == '=')
                        printf("menorouigual\n");

                    else if (c == '>')
                        printf("diferente\n");

                    else {

                        printf("menor\n");

                        ungetc(c, fp);
                        coluna--;
                    }

                    break;

                case '>':

                    c = fgetc(fp);
                    coluna++;

                    if (c == '=')
                        printf("maiorouigual\n");

                    else {

                        printf("maior\n");

                        ungetc(c, fp);
                        coluna--;
                    }

                    break;

                case ':':

                    c = fgetc(fp);
                    coluna++;

                    if (c == '=')
                        printf("atribuicao\n");

                    else {

                        printf("doispontos\n");

                        ungetc(c, fp);
                        coluna--;
                    }

                    break;

                case ')':
                    printf("fechaparenteses\n");
                    break;

                case '[':
                    printf("abrecolchetes\n");
                    break;

                case ']':
                    printf("fechacolchetes\n");
                    break;

                case ',':
                    printf("virgula\n");
                    break;

                case ';':
                    printf("pontoevirirgula\n");
                    break;

                case '.':
                    printf("ponto\n");
                    break;

                // ================= ERRO LÉXICO =================
                default:

                    erroLexico(
                        "simbolo desconhecido",
                        c
                    );
            }
        }
    }

    // ================= FECHAMENTO DE ARQUIVO =================
    fclose(fp);

    printf("\n========== ANALISE FINALIZADA ==========\n");

    return 0;
}