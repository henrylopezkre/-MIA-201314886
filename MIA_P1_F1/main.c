#include <stdlib.h>
#include "execute.h"

void main(){
	char instruccion[TAMANIO_INSTRUCCION];
	while(true){
        printf("MIA@P1_F1:~ $ ");
        fgets(instruccion, TAMANIO_INSTRUCCION, stdin);
        if(!strcmp(instruccion, "exit\n")){
            break;
        }else if(!strcmp(instruccion, "clear\n")){
            system("clear");
            continue;
        }
        analizador(instruccion);
    }
}
