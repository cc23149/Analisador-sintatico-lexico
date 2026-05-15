/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>

// função para identificar palavras reservadas
void palavraReservada(char *str) {
    if (strcmp(str, "program") == 0) printf("programa\n");
    else if (strcmp(str, "label") == 0) printf("rotulo\n");
    else if (strcmp(str, "type") == 0) printf("tipo\n");
    else if (strcmp(str, "var") == 0) printf("variavel\n");
    else if (strcmp(str, "procedure") == 0) printf("procedimento\n");
    else if (strcmp(str, "function") == 0) printf("funcao\n");
    else if (strcmp(str, "begin") == 0) printf("inicio\n");
    else if (strcmp(str, "end") == 0) printf("fim\n");
    else if (strcmp(str, "if") == 0) printf("se\n");
    else if (strcmp(str, "then") == 0) printf("entao\n");
    else if (strcmp(str, "else") == 0) printf("senao\n");
    else if (strcmp(str, "while") == 0) printf("enquanto\n");
    else if (strcmp(str, "do") == 0) printf("faca\n");
    else if (strcmp(str, "goto") == 0) printf("vapara\n");
    else if (strcmp(str, "and") == 0) printf("e\n");
    else if (strcmp(str, "or") == 0) printf("ou\n");
    else if (strcmp(str, "not") == 0) printf("nao\n");
    else if (strcmp(str, "div") == 0) printf("dividir\n");
    else printf("identificador\n");
}

int main() {
    FILE *fp = fopen("entrada.txt", "r");

    char c;
    char buffer[100];
    int i;

    if (fp == NULL) {
        printf("Erro ao abrir arquivo\n");
        return 1;
    }

    while ((c = fgetc(fp)) != EOF) {

        // ignorar espaços
        if (isspace(c))
            continue;

        // palavras reservadas ou identificadores
        if (isalpha(c)) {

            i = 0;
            buffer[i++] = c;

            while (isalnum(c = fgetc(fp)) || c == '_') {
                buffer[i++] = c;
            }

            buffer[i] = '\0';

            palavraReservada(buffer);

            ungetc(c, fp);
        }

        // números
        else if (isdigit(c)) {

            while (isdigit(c = fgetc(fp)));

            printf("numero\n");

            ungetc(c, fp);
        }

        // operadores e símbolos
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

                    if (c == '=')
                        printf("menorouigual\n");
                    else if (c == '>')
                        printf("diferente\n");
                    else {
                        printf("menor\n");
                        ungetc(c, fp);
                    }
                    break;

                case '>':
                    c = fgetc(fp);

                    if (c == '=')
                        printf("maiorouigual\n");
                    else {
                        printf("maior\n");
                        ungetc(c, fp);
                    }
                    break;

                case ':':
                    c = fgetc(fp);

                    if (c == '=')
                        printf("atribuicao\n");
                    else {
                        printf("doispontos\n");
                        ungetc(c, fp);
                    }
                    break;

                case '(':
                    printf("abreparenteses\n");
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
                    printf("pontoevirgula\n");
                    break;

                case '.':
                    printf("ponto\n");
                    break;

                default:
                    printf("desconhecido\n");
                    break;
            }
        }
    }

    fclose(fp);
    return 0;
}
