#include <stdio.h>

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
        consome(NUMERO, "esperava label numerico");
    }

    // IDENTIFICADOR
    else if (tokenAtual == IDENTIFICADOR) {

        avancaToken();

        // ATRIBUIÇÃO
        if (tokenAtual == ATRIBUICAO) {

            avancaToken();
            compila_expressao();
        }

        // CHAMADA DE PROCEDURE/FUNÇÃO
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

    // LABEL
    if (tokenAtual == LABEL)
        compila_label();

    // VAR
    if (tokenAtual == VARIAVEL)
        compila_var();

    // PROCEDURES E FUNCTIONS
    while (tokenAtual == PROCEDURE || tokenAtual == FUNCTION) {

        if (tokenAtual == PROCEDURE)
            compila_procedure();

        else
            compila_function();
    }

    // BEGIN
    consome(BEGIN_TOK