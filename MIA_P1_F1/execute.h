#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "structs.h"
#include "parts.h"

void a_minusculas(char *instruccion);
void analizador(char *instruccion);
void crear_estructura_de_ejecucion();
void quitar_salto_linea(char *instruccion);
void verificar_continuacion_instruccion(char *instruccion);
void borrar_comentario_final(char *instruccion);
void concatenar_instruccion_completa(char *instruccion);
void reiniciar_instruccion_completa();

int get_numero_particion(char *name, char letra_disco);
char get_letra_disco(char *path);
bool existeParticion(char *name);
bool existeDisco(char *path);
char* get_path(char *id);
bool existeMontaje(char *id);

void ejecutar(S_Instruccion *s_inst);
MBR crearMBR(int num_bytes);
EBR crearEBR();
void listarEBRs(char *path, int inicia_ebr);
bool existe_part_logica(char *path, int inicia_ebr, char *nombre);
int get_inicio_extendida(char *path);
EBR get_ebr(char *path, int inicia_ebr, char *nombre);
int cantidad_particiones_logicas(char *path);

bool verificar_id(char *id);
char get_tipo_particion(char *path_disco, char *nombre);
int get_inicio_particion(char *path_disco, char tipo_particion, char *nombre);
int get_size_particion(char *path_disco, char tipo_particion, char *nombre);
int get_inicio_real_part(int byte_inicio_particion, bool es_logica);
int get_tamanio_real_part(int tamanio_particion, bool es_logica);
char get_ajuste_particion(char *path_disco, char tipo_particion, char *nombre);

#define TAMANIO_INSTRUCCION 4096
#define TAMANIO_TOKEN 256
char instruccion_completa[TAMANIO_INSTRUCCION];
bool esperar_siguiente_linea = false;
EncabezadoMontaje montaje[10];
int cont_part = 0;
List parts;

void a_minusculas(char *cadena){
    bool omitir_seccion = false;
    bool omitir_comillas = false;
    for (int i = 0; i < TAMANIO_INSTRUCCION; i++){
        char nombre_parametro[] = {cadena[i], tolower(cadena[i+1]), tolower(cadena[i+2]), tolower(cadena[i+3]), tolower(cadena[i+4]), cadena[i+5], cadena[i+6]};
        if(!strncmp(nombre_parametro, "-path::", 7) || !strncmp(nombre_parametro, "-name::", 7)){
            cadena[i+1] = tolower(cadena[i+1]);
            cadena[i+2] = tolower(cadena[i+2]);
            cadena[i+3] = tolower(cadena[i+3]);
            cadena[i+4] = tolower(cadena[i+4]);
            omitir_seccion = true;
            i = i + 6;
            if(cadena[i] == '\"'){
                omitir_comillas = true;
                i++;
            }
        }
        if(omitir_seccion){
            if(cadena[i] == ' ' && omitir_comillas == false){
                omitir_seccion = false;
            }else if(cadena[i] == '\"' && omitir_comillas){
                omitir_seccion = false;
                omitir_comillas = false;
            }
            continue;
        }
        cadena[i] = tolower(cadena[i]);
    }
}

void analizador(char *instruccion){
    quitar_salto_linea(instruccion);
    verificar_continuacion_instruccion(instruccion);
    if(esperar_siguiente_linea == false){
        if(instruccion[0] == '#'){
            return;
        }else{
            borrar_comentario_final(instruccion);
        }
        concatenar_instruccion_completa(instruccion);
        a_minusculas(instruccion_completa);
        crear_estructura_de_ejecucion();
        reiniciar_instruccion_completa();
    }else{
        concatenar_instruccion_completa(instruccion);
    }
}
void quitar_salto_linea(char *instruccion){
    for(int i = 0; i < TAMANIO_INSTRUCCION; i++){
        if(instruccion[i] == '\n'){
            instruccion[i] = '\0';
            break;
        }
    }
}
void verificar_continuacion_instruccion(char *instruccion){
    for(int i = 0; i < TAMANIO_INSTRUCCION; i++){
        if(instruccion[i] == '\\' && instruccion[i+1] == '\0' || instruccion[i] == '\\' && instruccion[i+1] == ' '){
            instruccion[i] = '\0';
            esperar_siguiente_linea = true;
            return;
        }
    }
    esperar_siguiente_linea = false;
}
void borrar_comentario_final(char *instruccion){
    bool inicio_comentario = false;
    for(int i = 0; i < TAMANIO_INSTRUCCION; i++){
        if(instruccion[i] == '#'){
            inicio_comentario = true;
        }
        if(inicio_comentario){
            instruccion[i] = '\0';
        }
    }
}
void concatenar_instruccion_completa(char *instruccion){
    strcat(instruccion_completa, instruccion);
}
void reiniciar_instruccion_completa(){
    for(int i = 0; i < TAMANIO_INSTRUCCION; i++){
        instruccion_completa[i] = '\0';
    }
}
void reiniciar_token(char *tk){
    for(int i = 0; i < TAMANIO_TOKEN; i++){
        tk[i] = '\0';
    }
}
void crear_estructura_de_ejecucion(){
    printf("\n");
    char vec_tokens[16][TAMANIO_TOKEN];
    for(int i = 0; i < 16; i++){
        reiniciar_token(vec_tokens[i]);
    }
    int num_token = 0;
    char token_aux[TAMANIO_TOKEN];
    reiniciar_token(token_aux);
    bool leyendo_token = false;
    bool fin_leer_token = false;
    bool saltar_comillas = false;
    for (int i = 0; i < TAMANIO_INSTRUCCION; ++i){
        if(instruccion_completa[i] != ' ' && instruccion_completa[i] != '\0'){
            if(instruccion_completa[i] == '\"' && saltar_comillas == false){
                saltar_comillas = true;
            }else if(instruccion_completa[i] == '\"' && saltar_comillas == true){
                saltar_comillas = false;
            }
            if(leyendo_token == true){
                char caracter[2];
                caracter[1] = '\0';
                caracter[0] = instruccion_completa[i];
                strcat(token_aux,caracter);
            }else{
                token_aux[0] = instruccion_completa[i];
                leyendo_token = true;
            }
        }else{
            if(instruccion_completa[i] == ' ' && !leyendo_token){
                continue;
            }
            if(saltar_comillas == true){
                if(instruccion_completa[i] = ' '){
                    strcat(token_aux," ");
                }
            }else{
                fin_leer_token = true;
            }
        }
        if(fin_leer_token == true && leyendo_token == true){
            leyendo_token = false;
            fin_leer_token = false;
            strcpy(vec_tokens[num_token],token_aux);
            reiniciar_token(token_aux);
            num_token++;
        }
    }
    bool error_crear_estructura_inst = false;
    S_Instruccion s_inst;
    s_inst.tiene_path = false;
    s_inst.tiene_size = false;
    s_inst.tiene_unit = false;
    s_inst.tiene_type = false;
    s_inst.tiene_delete = false;
    s_inst.tiene_name = false;
    s_inst.tiene_fit = false;
    s_inst.tiene_add = false;
    s_inst.tiene_id = false;
    for(int i = 0; i < 16; i++){
        if(vec_tokens[i][0] != '\0'){
            if(i == 0){
                if(!strcmp(vec_tokens[i],"mkdisk")){
                    strcpy(s_inst.comando, vec_tokens[i]);
                }else if(!strcmp(vec_tokens[i],"rmdisk")){
                    strcpy(s_inst.comando, vec_tokens[i]);
                }else if(!strcmp(vec_tokens[i],"fdisk")){
                    strcpy(s_inst.comando, vec_tokens[i]);
                }else if(!strcmp(vec_tokens[i],"mount")){
                    strcpy(s_inst.comando, vec_tokens[i]);
                }else if(!strcmp(vec_tokens[i],"umount")){
                    strcpy(s_inst.comando, vec_tokens[i]);
                    initlist(&parts);
                }else if(!strcmp(vec_tokens[i],"rep")){
                    strcpy(s_inst.comando, vec_tokens[i]);
                }else if(!strcmp(vec_tokens[i],"exec")){
                    strcpy(s_inst.comando, vec_tokens[i]);
                }else{
                    printf("E: comando desconocido '%s' \n",vec_tokens[i]);
                    error_crear_estructura_inst = true;
                    break;
                }
            }else{
                if(!strncmp(vec_tokens[i],"-path::\"", 8)){
                    s_inst.tiene_path = true;
                    memcpy(s_inst.path,&vec_tokens[i][8],strlen(vec_tokens[i]));
                    s_inst.path[strlen(s_inst.path)-1] = '\0';
                }else if(!strncmp(vec_tokens[i],"+unit::", 7)){
                    if((vec_tokens[i][7] == 'b' || vec_tokens[i][7] == 'k' || vec_tokens[i][7] == 'm') && vec_tokens[i][8] == '\0'){
                        s_inst.tiene_unit = true;
                        s_inst.unit = vec_tokens[i][7];
                    }else{
                        error_crear_estructura_inst = true;
                        printf("E: el parámetro +unit no acepta valor desconocido '%s'\n",vec_tokens[i]);
                    }
                }else if(!strncmp(vec_tokens[i],"-size::", 7)){
                    if(vec_tokens[i][7] == '-'){
                        error_crear_estructura_inst = true;
                        printf("E: el parámetro -size no acepta valores menores a 0, %s\n", vec_tokens[i]);
                    }else{
                        s_inst.tiene_size = true;
                        int fin_num;
                        for(int n = 0; n < TAMANIO_TOKEN; n++){
                            if(vec_tokens[i][n] == '\0'){
                                fin_num = n;
                            }
                        }
                        char char_num[256];
                        memcpy(char_num, &vec_tokens[i][7], fin_num); //Hace un substring
                        s_inst.size = atoi(char_num);
                        if(s_inst.size <= 0){
                            error_crear_estructura_inst = true;
                            printf("E: el parámetro -size acepta valores mayores a 0, %s\n", vec_tokens[i]);
                        }
                    }
                }else if(!strncmp(vec_tokens[i],"-name::\"", 8)){
                    s_inst.tiene_name = true;
                    memcpy(s_inst.name,&vec_tokens[i][8],strlen(vec_tokens[i]));
                    s_inst.name[strlen(s_inst.name)-2] = '\0';
                }else if(!strncmp(vec_tokens[i],"-name::", 7)){
                    memcpy(s_inst.name, &vec_tokens[i][7], strlen(vec_tokens[i]));
                    s_inst.tiene_name = true;
                }else if(!strncmp(vec_tokens[i],"+type::", 7)){
                    if((vec_tokens[i][7] == 'p' || vec_tokens[i][7] == 'e' || vec_tokens[i][7] == 'l') && vec_tokens[i][8] == '\0'){
                        s_inst.tiene_type = true;
                        s_inst.type = vec_tokens[i][7];
                    }else{
                        error_crear_estructura_inst = true;
                        printf("E: el parámetro +type no acepta valor desconocido '%s\'n",vec_tokens[i]);
                    }
                }else if(!strncmp(vec_tokens[i],"+fit::", 6)){
                    if((vec_tokens[i][6] == 'b' || vec_tokens[i][6] == 'f' || vec_tokens[i][6] == 'w') && vec_tokens[i][7] == '\0'){
                        s_inst.tiene_fit = true;
                        s_inst.fit[0] = vec_tokens[i][6];
                    }else{
                        error_crear_estructura_inst = true;
                        printf("E: el parámetro +fit no acepta valor desconocido '%s'\n",vec_tokens[i]);
                    }
                }else if(!strncmp(vec_tokens[i],"+delete::", 9)){
                    if( ((vec_tokens[i][9] == 'f' && vec_tokens[i][10] == 'a' && vec_tokens[i][11] == 's' && vec_tokens[i][12] == 't') || (vec_tokens[i][9] == 'f' && vec_tokens[i][10] == 'u' && vec_tokens[i][11] == 'l' && vec_tokens[i][12] == 'l')) && vec_tokens[i][12] == '\0' ){
                        s_inst.tiene_delete = true;
                        memcpy(s_inst.delete, &vec_tokens[i][9], 4);
                    }else{
                        error_crear_estructura_inst = true;
                        printf("E: el parámetro -delete no acepta valor desconocido '%s'\n",vec_tokens[i]);
                    }
                }else if(!strncmp(vec_tokens[i],"+add::", 6)){
                    s_inst.tiene_add = true;
                    int fin_num;
                    for(int n = 0; n < TAMANIO_TOKEN; n++){
                        if(vec_tokens[i][n] == '\0'){
                            fin_num = n;
                        }
                    }
                    char char_num[256];
                    memcpy(char_num, &vec_tokens[i][6], fin_num);
                    s_inst.add = atoi(char_num);
                    if(s_inst.add == 0){
                        error_crear_estructura_inst = true;
                        printf("E: el parámetro +add acepta valores mayores a 0, %s\n", vec_tokens[i]);
                    }
                }else if(!strncmp(vec_tokens[i],"-id::", 5)){
                    memcpy(s_inst.id, &vec_tokens[i][5], strlen(vec_tokens[i]));
                    s_inst.tiene_id = true;
                }else if(verificar_id(vec_tokens[i])){
                    get_id_particiones(vec_tokens[i]);
                    s_inst.tiene_id = true;
                }else{
                    error_crear_estructura_inst = true;
                    printf("E: parámetro desconocido '%s'\n",vec_tokens[i]);
                }
            }
        }else{
            break;
        }
    }
    if(error_crear_estructura_inst == false){
        if(!strcmp(s_inst.comando, "exec")){
            if(access(s_inst.path, F_OK) == -1){
                printf("E: no existe la ruta o archivo especificado\n\n");
                return;
            }
            FILE *ptr_file;
            char buf[TAMANIO_TOKEN];
            ptr_file =fopen(s_inst.path,"r");
            reiniciar_instruccion_completa();
            while (fgets(buf,TAMANIO_TOKEN, ptr_file)!=NULL){
                if(buf[0] == '\n'){
                    continue;
                }
                char instruccion_archivo[TAMANIO_INSTRUCCION];
                strcpy(instruccion_archivo, buf);
                printf("Leyendo... %s",instruccion_archivo);
                analizador(instruccion_archivo);
            }
            fclose(ptr_file);
        }else{
            printf("Ejecutando...\n");
            segunda_fase_analisis(&s_inst);
        }
    }
    printf("\n\n");
}

bool verificar_id(char *id)
{
    bool ver = false;
    char aux[10];
    strcpy(aux, id);
    strtok(aux, "::");
    if(strlen(aux) == 4){
        if(id[0] == '-' && id[1] == 'i' && id[2] == 'd' && isdigit(id[3])){
            ver = true;
        }
    }else if(strlen(aux) == 5){
        if(id[0] == '-' && id[1] == 'i' && id[2] == 'd' && isdigit(id[3]) && isdigit(id[4])){
            ver = true;
        }
    }
    return ver;
}
void get_id_particiones(char *id)
{
    char part[10];
    char tkn[10];
    strcpy(tkn, id);
    strtok(tkn, "::");
    int len = strlen(tkn) + 2;
    memcpy(part, &id[len], strlen(id)-len);
    part[strlen(id)-len] = '\0';
    insertback(&parts,part);
}

void segunda_fase_analisis(S_Instruccion *s_inst){
    bool error_segundo_analisis = false;
    if(!strcmp(s_inst->comando, "mkdisk")){
        if(s_inst->tiene_size == true && s_inst->tiene_path == true && s_inst->tiene_name == true){
            if(s_inst->tiene_unit == true){
                if(!(s_inst->unit == 'k' || s_inst->unit == 'm')){
                    error_segundo_analisis = true;
                    printf("E: parámetro desconocido '%c' \n", s_inst->unit);
                    printf("Use: k|K| = kilobytes o m|M = megabytes \n");
                }else{
                    if(s_inst->unit == 'k' && s_inst->tiene_unit == true){
                        if(s_inst->size < 10240){
                            error_segundo_analisis = true;
                            printf("E: tamaño mínimo de disco debe ser 10Mb -> 10240Kb \n");
                        }
                    }else if (s_inst->unit == 'm' || s_inst->tiene_unit == false){
                        if(s_inst->size < 10){
                            error_segundo_analisis = true;
                            printf("E: tamaño mínimo de disco debe ser 10Mb -> 10240Kb \n");
                        }
                    }
                }
            }
        }else{
            error_segundo_analisis = true;
            printf("E: faltan parametros obligatorios para el comando mkdisk\n");
        }
    }else if(!strcmp(s_inst->comando, "rmdisk")){
        if(s_inst->tiene_path == false){
            error_segundo_analisis = true;
            printf("E: faltan parámetros obligatorios '-path'\n");
        }
    }else if(!strcmp(s_inst->comando, "fdisk")){
        if(s_inst->tiene_path == false || s_inst->tiene_name == false){
            error_segundo_analisis = true;
            printf("E: faltan parámetros obligatorios '-name'|'-path'\n");
        }else if(s_inst->tiene_path){
            if(access(s_inst->path, F_OK) == -1){
                error_segundo_analisis = true;
                printf("E: no existe la ruta o archivo especificado\n");
            }
        }
    }else if(!strcmp(s_inst->comando, "mount")){
         if(s_inst->tiene_path == false || s_inst->tiene_name == false){
            error_segundo_analisis = true;
            printf("E: faltan parámetros obligatorios '-name'|'-path'\n");
        }else if(s_inst->tiene_path){
            if(access(s_inst->path, F_OK) == -1){
                error_segundo_analisis = true;
                printf("E: no existe la ruta o archivo especificado\n");
            }
        }
    }else if(!strcmp(s_inst->comando, "umount")){
        if(s_inst->tiene_id == false){
            error_segundo_analisis = true;
            printf("E: faltan parámetros obligatorios '-id'\n");
        }
    }else if(!strcmp(s_inst->comando, "rep")){
        if(s_inst->tiene_path == false || s_inst->tiene_name == false || s_inst->tiene_id == false){
            error_segundo_analisis = true;
            printf("E: faltan parámetros obligatorios '-name'|'-path'|'-id'\n");
        }
    }else if(!strcmp(s_inst->comando, "exec")){
        if(s_inst->tiene_path == false){
            error_segundo_analisis = true;
            printf("E: faltan parámetros obligatorios '-path'\n");
        }else{
            if(access(s_inst->path, F_OK) == -1){
                error_segundo_analisis = true;
                printf("E: no existe la ruta o archivo especificado\n");
            }
        }
    }
    if(error_segundo_analisis == false){
        ejecutar(s_inst);
    }
}
void reiniciar_path(char *path){
    for(int i = 0; i < 256; i++){
        path[i] = '\0';
    }
}
void ejecutar(S_Instruccion *s_inst){
    if(!strcmp(s_inst->comando, "mkdisk")){
        char aux_path[256];
        reiniciar_path(aux_path);
        aux_path[0] = '/';
        char full_path[256];
        reiniciar_path(full_path);
        char *tkn;
        strcpy(full_path, s_inst->path);
        strcat(full_path, s_inst->name);
        tkn = strtok(full_path, "/");
        while(tkn != NULL){
            strcat(aux_path, tkn);
            if(tkn[strlen(tkn)-4] == '.' && tkn[strlen(tkn)-3] == 'd' && tkn[strlen(tkn)-2] == 's' && tkn[strlen(tkn)-1] == 'k'){
                char buffer = '\0';
                FILE *disco = fopen(aux_path, "wb+");
                int num_bytes = 0;
                if(s_inst->unit == 'k' && s_inst->tiene_unit){
                    num_bytes = s_inst->size*1024;
                }else if (s_inst->unit == 'm' || s_inst->tiene_unit == false){
                    num_bytes = s_inst->size*1024*1024;
                }
                for(int i = 0; i < num_bytes; i++){
                    fwrite(&buffer,sizeof(buffer),1,disco);
                }
                fclose(disco);
                disco = fopen(aux_path, "rb+");
                MBR mbr = crearMBR(num_bytes);
                fwrite(&mbr, sizeof(MBR), 1, disco);
                fclose(disco);
                printf("Se creó el disco: %s%s\n", s_inst->path, s_inst->name);
            }else{
                if(access(aux_path, F_OK) == -1){
                    mkdir(aux_path, 0700);
                    strcat(aux_path,"/");
                }else{
                    strcat(aux_path,"/");
                }
            }
            tkn = strtok(NULL, "/");
        }
        reiniciar_path(aux_path);
    }else if(!strcmp(s_inst->comando, "rmdisk")){
        if(access(s_inst->path, F_OK) != -1){
            char confirmacion;
            printf("¿Desea eliminar el disco? [S/N]\n");
            scanf(" %c", &confirmacion);
            getc(stdin);
            if(confirmacion == 's' || confirmacion == 'S'){
                char full_path[256];
                reiniciar_path(full_path);
                full_path[0] = '/';
                strcpy(full_path, s_inst->path);
                strcat(full_path, s_inst->name);
                remove(full_path);
                printf("Se eliminó el disco: %s\n", full_path);
            }else{
                printf("W: no se realizaron cambios.\n");
            }

        }else{
            printf("E: no existe la ruta o archivo especificado\n");
        }
    }else if(!strcmp(s_inst->comando, "fdisk")){
        if(s_inst->tiene_size){
            if(s_inst->tiene_type == false){
                s_inst->tiene_type = true;
                s_inst->type = 'p';
            }
            if(s_inst->tiene_fit == false){
                s_inst->fit[0] = 'w';
            }
            if(s_inst->type == 'p' || s_inst->type == 'e'){
                bool continuar_creacion = true;
                FILE *disco;
                disco = fopen(s_inst->path, "rb");
                MBR mbr_lectura;
                rewind(disco);
                fread(&mbr_lectura, sizeof(MBR), 1, disco);
                int contador_primarias = 0;
                int contador_extendida = 0;
                char nombres_particiones[4][16];
                if(mbr_lectura.mbr_partition_1.part_status == '1'){
                    if(mbr_lectura.mbr_partition_1.part_type == 'p'){
                        contador_primarias++;
                    }else{
                        contador_extendida++;
                    }
                    strcpy(nombres_particiones[0], mbr_lectura.mbr_partition_1.part_name);
                }
                if(mbr_lectura.mbr_partition_2.part_status == '1'){
                    if(mbr_lectura.mbr_partition_2.part_type == 'p'){
                        contador_primarias++;
                    }else{
                        contador_extendida++;
                    }
                    strcpy(nombres_particiones[1], mbr_lectura.mbr_partition_2.part_name);
                }
                if(mbr_lectura.mbr_partition_3.part_status == '1'){
                    if(mbr_lectura.mbr_partition_3.part_type == 'p'){
                        contador_primarias++;
                    }else{
                        contador_extendida++;
                    }
                    strcpy(nombres_particiones[2], mbr_lectura.mbr_partition_3.part_name);
                }
                if(mbr_lectura.mbr_partition_4.part_status == '1'){
                    if(mbr_lectura.mbr_partition_4.part_type == 'p'){
                        contador_primarias++;
                    }else{
                        contador_extendida++;
                    }
                    strcpy(nombres_particiones[3], mbr_lectura.mbr_partition_4.part_name);
                }
                if((contador_primarias + contador_extendida) == 4){
                    printf("E: el disco ya alcanzó el limite de particiones (4)\n");
                    continuar_creacion = false;
                }else if(s_inst->type == 'e' && contador_extendida == 1){
                    printf("E: el disco ya posee una particion extendida\n");
                    continuar_creacion = false;
                }
                for(int i = 0; i < 4; i++){
                    if(!strcmp(nombres_particiones[i],s_inst->name)){
                        printf("E: nombre ya esta en uso (%s)\n", s_inst->name);
                        continuar_creacion = false;
                    }
                }
                if(continuar_creacion == false){
                    fclose(disco);
                    return;
                }
                int bytes_inicios[4];
                int bytes_tamanios[4];

                if(mbr_lectura.mbr_partition_1.part_status == '1'){
                    bytes_inicios[0] = mbr_lectura.mbr_partition_1.part_start;
                    bytes_tamanios[0] = mbr_lectura.mbr_partition_1.part_size;
                }else{
                    bytes_inicios[0] = -1;
                }

                if(mbr_lectura.mbr_partition_2.part_status == '1'){
                    bytes_inicios[1] = mbr_lectura.mbr_partition_2.part_start;
                    bytes_tamanios[1] = mbr_lectura.mbr_partition_2.part_size;
                }else{
                    bytes_inicios[1] = -1;
                }

                if(mbr_lectura.mbr_partition_3.part_status == '1'){
                    bytes_inicios[2] = mbr_lectura.mbr_partition_3.part_start;
                    bytes_tamanios[2] = mbr_lectura.mbr_partition_3.part_size;
                }else{
                    bytes_inicios[2] = -1;
                }

                if(mbr_lectura.mbr_partition_4.part_status == '1'){
                    bytes_inicios[3] = mbr_lectura.mbr_partition_4.part_start;
                    bytes_tamanios[3] = mbr_lectura.mbr_partition_4.part_size;
                }else{
                    bytes_inicios[3] = -1;
                }
                int n = 4;
                for(int i = 1; i < n; i++){
                    for(int j = 0; j < n-1; j++){
                        if(bytes_inicios[j] > bytes_inicios[j+1]){
                            int aux = bytes_inicios[j];
                            bytes_inicios[j] = bytes_inicios[j+1];
                            bytes_inicios[j+1] = aux;
                            int aux2 = bytes_tamanios[j];
                            bytes_tamanios[j] = bytes_tamanios[j+1];
                            bytes_tamanios[j+1] = aux2;
                        }
                    }
                }
                int espacio_entre_particiones[5] = {-1,-1,-1,-1,-1};
                int iniciar_en[5] = {-1,-1,-1,-1,-1};
                int contador_espacio = 0;
                bool primara_part_encontrada = false;
                for(int i = 0; i < 4; i++){
                    if(bytes_inicios[i] != -1){
                        if(primara_part_encontrada == false){
                            espacio_entre_particiones[contador_espacio] = bytes_inicios[i] - (sizeof(MBR));
                            iniciar_en[contador_espacio] = (sizeof(MBR));
                            primara_part_encontrada = true;
                            if(i == 3){
                                contador_espacio++;
                                espacio_entre_particiones[contador_espacio] = (mbr_lectura.mbr_tamanio) - (bytes_inicios[i]+bytes_tamanios[i]);
                                iniciar_en[contador_espacio] = (bytes_inicios[i]+bytes_tamanios[i]);
                            }
                        }else{
                            if(i != 3){
                                espacio_entre_particiones[contador_espacio] = bytes_inicios[i] - (bytes_inicios[i-1]+bytes_tamanios[i-1]);
                                iniciar_en[contador_espacio] = (bytes_inicios[i-1]+bytes_tamanios[i-1]);
                            }else{
                                espacio_entre_particiones[contador_espacio] = bytes_inicios[i] - (bytes_inicios[i-1]+bytes_tamanios[i-1]);
                                iniciar_en[contador_espacio] = (bytes_inicios[i-1]+bytes_tamanios[i-1]);
                                contador_espacio++;
                                espacio_entre_particiones[contador_espacio] = (mbr_lectura.mbr_tamanio) - (bytes_inicios[i]+bytes_tamanios[i]);
                                iniciar_en[contador_espacio] = (bytes_inicios[i]+bytes_tamanios[i]);
                            }
                        }
                        contador_espacio++;
                    }
                }
                int byte_inicio = 0;
                bool hay_espacio_disponible = false;
                if(primara_part_encontrada == false){
                    espacio_entre_particiones[0] = (mbr_lectura.mbr_tamanio-sizeof(MBR));
                    iniciar_en[0] = (sizeof(MBR));
                    byte_inicio = (sizeof(MBR));
                    int espacio_entre = espacio_entre_particiones[0];
                    int tamanio_en_bytes = s_inst->size;
                    primara_part_encontrada = true;
                }
                if(primara_part_encontrada){
                    for(int i = 0; i < 5; i++){
                        int espacio_entre = espacio_entre_particiones[i];
                        int tamanio_en_bytes = s_inst->size;
                        if(s_inst->tiene_unit){
                            if(s_inst->unit == 'b'){
                                if(espacio_entre >= tamanio_en_bytes){
                                    hay_espacio_disponible = true;
                                    byte_inicio = iniciar_en[i];
                                    break;
                                }
                            }else if(s_inst->unit == 'k'){
                                if(espacio_entre >= (tamanio_en_bytes * 1024)){
                                    hay_espacio_disponible = true;
                                    byte_inicio = iniciar_en[i];
                                    break;
                                }
                            }else if(s_inst->unit == 'm'){
                                if(espacio_entre >= (tamanio_en_bytes * 1024 * 1024)){
                                    hay_espacio_disponible = true;
                                    byte_inicio = iniciar_en[i];
                                    break;
                                }
                            }
                        }else{
                            if(espacio_entre >= (tamanio_en_bytes * 1024)){
                                hay_espacio_disponible = true;
                                byte_inicio = iniciar_en[i];
                                break;
                            }
                        }
                    }
                }
                if(hay_espacio_disponible){
                    if(mbr_lectura.mbr_partition_1.part_status == '0'){
                        strcpy(mbr_lectura.mbr_partition_1.part_name,s_inst->name);
                        mbr_lectura.mbr_partition_1.part_status = '1';
                        mbr_lectura.mbr_partition_1.part_type = s_inst->type;
                        mbr_lectura.mbr_partition_1.part_fit = s_inst->fit[0];
                        mbr_lectura.mbr_partition_1.part_start = byte_inicio;
                        if(s_inst->tiene_unit){
                            if(s_inst->unit == 'b'){
                                mbr_lectura.mbr_partition_1.part_size = s_inst->size;
                            }else if(s_inst->unit == 'k'){
                                mbr_lectura.mbr_partition_1.part_size = s_inst->size * 1024;
                            }else if(s_inst->unit == 'm'){
                                mbr_lectura.mbr_partition_1.part_size = s_inst->size * 1024 * 1024;
                            }
                        }else{
                            mbr_lectura.mbr_partition_1.part_size = s_inst->size * 1024;
                        }
                    }else if(mbr_lectura.mbr_partition_2.part_status == '0'){
                        strcpy(mbr_lectura.mbr_partition_2.part_name,s_inst->name);
                        mbr_lectura.mbr_partition_2.part_status = '1';
                        mbr_lectura.mbr_partition_2.part_type = s_inst->type;
                        mbr_lectura.mbr_partition_2.part_fit = s_inst->fit[0];
                        mbr_lectura.mbr_partition_2.part_start = byte_inicio;
                        if(s_inst->tiene_unit){
                            if(s_inst->unit == 'b'){
                                mbr_lectura.mbr_partition_2.part_size = s_inst->size;
                            }else if(s_inst->unit == 'k'){
                                mbr_lectura.mbr_partition_2.part_size = s_inst->size * 1024;
                            }else if(s_inst->unit == 'm'){
                                mbr_lectura.mbr_partition_2.part_size = s_inst->size * 1024 * 1024;
                            }
                        }else{
                            mbr_lectura.mbr_partition_2.part_size = s_inst->size * 1024;
                        }
                    }else if(mbr_lectura.mbr_partition_3.part_status == '0'){
                        strcpy(mbr_lectura.mbr_partition_3.part_name,s_inst->name);
                        mbr_lectura.mbr_partition_3.part_status = '1';
                        mbr_lectura.mbr_partition_3.part_type = s_inst->type;
                        mbr_lectura.mbr_partition_3.part_fit = s_inst->fit[0];
                        mbr_lectura.mbr_partition_3.part_start = byte_inicio;
                        if(s_inst->tiene_unit){
                            if(s_inst->unit == 'b'){
                                mbr_lectura.mbr_partition_3.part_size = s_inst->size;
                            }else if(s_inst->unit == 'k'){
                                mbr_lectura.mbr_partition_3.part_size = s_inst->size * 1024;
                            }else if(s_inst->unit == 'm'){
                                mbr_lectura.mbr_partition_3.part_size = s_inst->size * 1024 * 1024;
                            }
                        }else{
                            mbr_lectura.mbr_partition_3.part_size = s_inst->size * 1024;
                        }
                    }else if(mbr_lectura.mbr_partition_4.part_status == '0'){
                        strcpy(mbr_lectura.mbr_partition_4.part_name,s_inst->name);
                        mbr_lectura.mbr_partition_4.part_status = '1';
                        mbr_lectura.mbr_partition_4.part_type = s_inst->type;
                        mbr_lectura.mbr_partition_4.part_fit = s_inst->fit[0];
                        mbr_lectura.mbr_partition_4.part_start = byte_inicio;
                        if(s_inst->tiene_unit){
                            if(s_inst->unit == 'b'){
                                mbr_lectura.mbr_partition_4.part_size = s_inst->size;
                            }else if(s_inst->unit == 'k'){
                                mbr_lectura.mbr_partition_4.part_size = s_inst->size * 1024;
                            }else if(s_inst->unit == 'm'){
                                mbr_lectura.mbr_partition_4.part_size = s_inst->size * 1024 * 1024;
                            }
                        }else{
                            mbr_lectura.mbr_partition_4.part_size = s_inst->size * 1024;
                        }
                    }
                    fclose(disco);
                    int tamanio_a_crear = 0;
                    if(s_inst->tiene_unit){
                        if(s_inst->unit == 'b'){
                            tamanio_a_crear = s_inst->size;
                        }else if(s_inst->unit == 'k'){
                            tamanio_a_crear =  s_inst->size * 1024;
                        }else if(s_inst->unit == 'm'){
                            tamanio_a_crear =  s_inst->size * 1024 * 1024;
                        }
                    }else{
                        tamanio_a_crear =  s_inst->size * 1024;
                    }
                    if(s_inst->type == 'e'){
                        if(tamanio_a_crear < sizeof(EBR)){
                            printf("E: el tamaño de la particion extendida es muy pequeño, se requieren al menos: %d Bytes\n", sizeof(EBR));
                            return;
                        }else{
                            EBR ebr_inicio;
                            ebr_inicio = crearEBR();
                            ebr_inicio.part_start = byte_inicio;
                            ebr_inicio.part_status = '0'; //FASE2
                            disco = fopen(s_inst->path, "rb+");
                            fseek(disco, byte_inicio, SEEK_SET);
                            fwrite(&ebr_inicio, sizeof(EBR), 1, disco);
                            fclose(disco);
                        }
                    }
                    disco = fopen(s_inst->path, "rb+");
                    fseek(disco, 0, SEEK_SET);
                    fwrite(&mbr_lectura, sizeof(MBR), 1, disco);
                    fclose(disco);
                    /*printf("---Informacion del Disco---\n");
                    printf("Tamanio %d\n", mbr_lectura.mbr_tamanio);
                    printf("-------------Particion 1: \n");
                    printf("status : %c\n", mbr_lectura.mbr_partition_1.part_status);
                    printf("type : %c\n", mbr_lectura.mbr_partition_1.part_type);
                    printf("fit : %c\n", mbr_lectura.mbr_partition_1.part_fit);
                    printf("start : %d\n", mbr_lectura.mbr_partition_1.part_start);
                    printf("size : %d\n", mbr_lectura.mbr_partition_1.part_size);
                    printf("name : %s\n", mbr_lectura.mbr_partition_1.part_name);
                    printf("-------------Particion 2: \n");
                    printf("status : %c\n", mbr_lectura.mbr_partition_2.part_status);
                    printf("type : %c\n", mbr_lectura.mbr_partition_2.part_type);
                    printf("fit : %c\n", mbr_lectura.mbr_partition_2.part_fit);
                    printf("start : %d\n", mbr_lectura.mbr_partition_2.part_start);
                    printf("size : %d\n", mbr_lectura.mbr_partition_2.part_size);
                    printf("name : %s\n", mbr_lectura.mbr_partition_2.part_name);
                    printf("-------------Particion 3: \n");
                    printf("status : %c\n", mbr_lectura.mbr_partition_3.part_status);
                    printf("type : %c\n", mbr_lectura.mbr_partition_3.part_type);
                    printf("fit : %c\n", mbr_lectura.mbr_partition_3.part_fit);
                    printf("start : %d\n", mbr_lectura.mbr_partition_3.part_start);
                    printf("size : %d\n", mbr_lectura.mbr_partition_3.part_size);
                    printf("name : %s\n", mbr_lectura.mbr_partition_3.part_name);
                    printf("-------------Particion 4: \n");
                    printf("status : %c\n", mbr_lectura.mbr_partition_4.part_status);
                    printf("type : %c\n", mbr_lectura.mbr_partition_4.part_type);
                    printf("fit : %c\n", mbr_lectura.mbr_partition_4.part_fit);
                    printf("start : %d\n", mbr_lectura.mbr_partition_4.part_start);
                    printf("size : %d\n", mbr_lectura.mbr_partition_4.part_size);
                    printf("name : %s\n", mbr_lectura.mbr_partition_4.part_name);*/
                }else{
                    printf("E: no hay suficiente espacio libre para crear la partición\n");
                    fclose(disco);
                }
                for (int i = 0; i < 4; ++i){
                    for (int j = 0; j < 16; ++j)
                    {
                        nombres_particiones[i][j] = '\0';
                    }
                }
            }else{
                FILE *disco;
                disco = fopen(s_inst->path, "rb");
                MBR mbr_lectura;
                rewind(disco);
                fread(&mbr_lectura, sizeof(MBR), 1, disco);
                fclose(disco);
                bool tiene_extendida = false;
                int byte_inicio_extendida = 0;
                int tamanio_extendida = 0;
                if(mbr_lectura.mbr_partition_1.part_status == '1'){
                    if(mbr_lectura.mbr_partition_1.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_1.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_1.part_size;
                    }
                }
                if(mbr_lectura.mbr_partition_2.part_status == '1'){
                    if(mbr_lectura.mbr_partition_2.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_2.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_2.part_size;
                    }
                }
                if(mbr_lectura.mbr_partition_3.part_status == '1'){
                    if(mbr_lectura.mbr_partition_3.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_3.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_3.part_size;
                    }
                }
                if(mbr_lectura.mbr_partition_4.part_status == '1'){
                    if(mbr_lectura.mbr_partition_4.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_4.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_4.part_size;
                    }
                }

                if(tiene_extendida){
                    if(existe_part_logica(s_inst->path, byte_inicio_extendida,s_inst->name)){
                        printf("E: nombre ya esta siendo utilizado por una particion lógica\n");
                        return;
                    }
                    disco = fopen(s_inst->path, "rb");
                    EBR ebr_lectura;
                    fseek(disco, byte_inicio_extendida, SEEK_SET);
                    fread(&ebr_lectura, sizeof(EBR), 1, disco);
                    fclose(disco);

                    if(ebr_lectura.part_next == -1 && ebr_lectura.part_status == '0'){
                        int tamanio_solicitado = 0;
                        if(s_inst->tiene_unit){
                            if(s_inst->unit == 'b'){
                                tamanio_solicitado = s_inst->size;
                            }else if(s_inst->unit == 'k'){
                                tamanio_solicitado = s_inst->size * 1024;
                            }else if(s_inst->unit == 'm'){
                                tamanio_solicitado = s_inst->size * 1024 * 1024;
                            }
                        }else{
                            tamanio_solicitado = s_inst->size * 1024;
                        }
                        if(tamanio_solicitado <= (tamanio_extendida - sizeof(EBR)) && tamanio_solicitado > sizeof(EBR)){
                            ebr_lectura.part_status = '1';
                            ebr_lectura.part_fit = s_inst->fit[0];
                            ebr_lectura.part_start = byte_inicio_extendida;
                            ebr_lectura.part_size = tamanio_solicitado;
                            ebr_lectura.part_next = -1;
                            strcpy(ebr_lectura.part_name, s_inst->name);

                            disco = fopen(s_inst->path, "rb+");
                            fseek(disco, ebr_lectura.part_start, SEEK_SET);
                            fwrite(&ebr_lectura, sizeof(EBR), 1, disco);
                            fclose(disco);
                            printf("Se creó la partición logica con %d Bytes\n", ebr_lectura.part_size);
                        }else if(tamanio_solicitado <= sizeof(EBR)){
                            printf("E: el tamaño de la particion extendida es muy pequeño, se requieren al menos: %d Bytes\n", sizeof(EBR)+1);
                        }else{
                            printf("E: no hay suficiente espacio libre en la particion extendida\n");
                            printf("Espacio libre en particion extendida : %d Bytes\n", (tamanio_extendida - sizeof(EBR)));
                        }
                    }else{
                        disco = fopen(s_inst->path, "rb");
                        EBR ebr_actual;
                        fseek(disco, byte_inicio_extendida, SEEK_SET);
                        fread(&ebr_actual, sizeof(EBR), 1, disco);
                        fclose(disco);
                        int espacio_libre = 0;
                        int iniciar_en_byte = 0;
                        bool encontro_espacio = false;
                        do{
                            if(ebr_actual.part_next != -1 && ebr_actual.part_status == '0'){
                                espacio_libre = ebr_actual.part_next - ebr_actual.part_start;
                                iniciar_en_byte = ebr_actual.part_start;
                            }else if(ebr_actual.part_next == -1 && ebr_actual.part_status == '1'){
                                 espacio_libre = tamanio_extendida - (ebr_actual.part_start - byte_inicio_extendida) - ebr_actual.part_size; //Nuevo
                                iniciar_en_byte = ebr_actual.part_start + ebr_actual.part_size;
                            }else if(ebr_actual.part_next != -1 && ebr_actual.part_status == '1'){
                                espacio_libre = ebr_actual.part_next - (ebr_actual.part_start + ebr_actual.part_size);
                                iniciar_en_byte = ebr_actual.part_start + ebr_actual.part_size;
                            }
                            int tamanio_solicitado = 0;
                            if(s_inst->tiene_unit){
                                if(s_inst->unit == 'b'){
                                    tamanio_solicitado = s_inst->size;
                                }else if(s_inst->unit == 'k'){
                                    tamanio_solicitado = s_inst->size * 1024;
                                }else if(s_inst->unit == 'm'){
                                    tamanio_solicitado = s_inst->size * 1024 * 1024;
                                }
                            }else{
                                tamanio_solicitado = s_inst->size * 1024;
                            }
                            if(espacio_libre >= tamanio_solicitado){

                                encontro_espacio = true;
                                if(tamanio_solicitado <= sizeof(EBR)){
                                    printf("E: el tamaño de la particion extendida es muy pequeño, se requieren al menos: %d Bytes\n", sizeof(EBR)+1);
                                    break;
                                }
                                EBR ebr_nuevo = crearEBR();
                                ebr_nuevo.part_start = iniciar_en_byte;
                                ebr_nuevo.part_size = tamanio_solicitado;
                                ebr_nuevo.part_fit = s_inst->fit[0];
                                if(ebr_actual.part_next != -1 && ebr_actual.part_status == '0'){
                                    ebr_nuevo.part_next = ebr_actual.part_next;
                                }else if(ebr_actual.part_next == -1){
                                    ebr_actual.part_next = ebr_nuevo.part_start;
                                }else{
                                    int aux_next = ebr_actual.part_next;
                                    ebr_nuevo.part_next = aux_next;
                                    ebr_actual.part_next = ebr_nuevo.part_start;
                                }
                                disco = fopen(s_inst->path, "rb+");
                                fseek(disco, ebr_actual.part_start, SEEK_SET);
                                fwrite(&ebr_actual, sizeof(EBR), 1, disco);
                                fclose(disco);
                                strcpy(ebr_nuevo.part_name, s_inst->name);
                                disco = fopen(s_inst->path, "rb+");
                                fseek(disco, ebr_nuevo.part_start, SEEK_SET);
                                fwrite(&ebr_nuevo, sizeof(EBR), 1, disco);
                                fclose(disco);
                                printf("Se creó la partición logica con %d Bytes\n", ebr_nuevo.part_size);
                                break;
                            }else{
                                if(ebr_actual.part_next != -1){
                                    disco = fopen(s_inst->path, "rb");
                                    fseek(disco, ebr_actual.part_next, SEEK_SET);
                                    fread(&ebr_actual, sizeof(EBR), 1, disco);
                                    fclose(disco);
                                }else{
                                    break;
                                }
                            }
                        }while(true);

                        if(!encontro_espacio){
                            printf("E: no hay espacio suficiente en la particion extendida\n");
                            printf("Espacio libre en disco: %d\n", espacio_libre);
                        }
                    }
                    listarEBRs(s_inst->path, byte_inicio_extendida);
                }else{
                    printf("E: no existe una particion extendida en este disco\n");
                }
            }
        }else if(s_inst->tiene_delete){
            if(s_inst->tiene_type == false){
                s_inst->tiene_type = true;
                s_inst->type = 'p';

                bool esLogica = existe_part_logica(s_inst->path, get_inicio_extendida(s_inst->path), s_inst->name);
                if(esLogica){
                    s_inst->type = 'l';
                }
            }
            if(existeParticion(s_inst->name)){
                printf("E: la particion se encuentra montada\n");
                return;
            }
            char confirmacion;
            printf("¿Desea eliminar la particion? [S/N]\n");
            scanf(" %c", &confirmacion);
            getc(stdin);
            if(confirmacion == 's' || confirmacion == 'S'){
                printf("Iniciando eliminacion...\n");
            }else{
                printf("W: No se realizaron cambios.\n");
                return;
            }
            if(s_inst->type == 'p' ||s_inst->type == 'e'){
                if(!strncmp(s_inst->delete, "fast",4)){
                    FILE *disco;
                    disco = fopen(s_inst->path, "rb");
                    MBR mbr_lectura;
                    rewind(disco);
                    fread(&mbr_lectura, sizeof(MBR), 1, disco);
                    fclose(disco);
                    bool particion_eliminada = true;
                    if(mbr_lectura.mbr_partition_1.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_1.part_name,s_inst->name)){
                        mbr_lectura.mbr_partition_1.part_status = '0';
                    }else if(mbr_lectura.mbr_partition_2.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_2.part_name,s_inst->name)){
                        mbr_lectura.mbr_partition_2.part_status = '0';
                    }else if(mbr_lectura.mbr_partition_3.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_3.part_name,s_inst->name)){
                        mbr_lectura.mbr_partition_3.part_status = '0';
                    }else if(mbr_lectura.mbr_partition_4.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_4.part_name,s_inst->name)){
                        mbr_lectura.mbr_partition_4.part_status = '0';
                    }else{
                        particion_eliminada = false;
                        printf("E: no existe la particion especificada (%s)\n", s_inst->name);
                    }
                    if(particion_eliminada){
                        disco = fopen(s_inst->path, "rb+");
                        fseek(disco, 0, SEEK_SET);
                        fwrite(&mbr_lectura, sizeof(MBR), 1, disco);
                        fclose(disco);
                        printf("Se eliminó la particion (%s) en modo Fast\n", s_inst->name);
                        /*printf("---Informacion del Disco---\n");
                        printf("Tamanio %d\n", mbr_lectura.mbr_tamanio);
                        printf("-------------Particion 1: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_1.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_1.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_1.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_1.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_1.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_1.part_name);
                        printf("-------------Particion 2: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_2.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_2.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_2.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_2.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_2.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_2.part_name);
                        printf("-------------Particion 3: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_3.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_3.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_3.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_3.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_3.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_3.part_name);
                        printf("-------------Particion 4: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_4.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_4.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_4.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_4.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_4.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_4.part_name);*/
                    }
                }else if(!strncmp(s_inst->delete, "full",4)){
                    FILE *disco;
                    disco = fopen(s_inst->path, "rb");
                    MBR mbr_lectura;
                    rewind(disco);
                    fread(&mbr_lectura, sizeof(MBR), 1, disco);
                    fclose(disco);
                    bool particion_eliminada = true;
                    int pos_byte = 0;
                    int num_bytes = 0;
                    if(mbr_lectura.mbr_partition_1.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_1.part_name,s_inst->name)){
                        mbr_lectura.mbr_partition_1.part_status = '0';
                        pos_byte = mbr_lectura.mbr_partition_1.part_start;
                        num_bytes = mbr_lectura.mbr_partition_1.part_size;
                    }else if(mbr_lectura.mbr_partition_2.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_2.part_name,s_inst->name)){
                        mbr_lectura.mbr_partition_2.part_status = '0';
                        pos_byte = mbr_lectura.mbr_partition_2.part_start;
                        num_bytes = mbr_lectura.mbr_partition_2.part_size;
                    }else if(mbr_lectura.mbr_partition_3.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_3.part_name,s_inst->name)){
                        mbr_lectura.mbr_partition_3.part_status = '0';
                        pos_byte = mbr_lectura.mbr_partition_3.part_start;
                        num_bytes = mbr_lectura.mbr_partition_3.part_size;
                    }else if(mbr_lectura.mbr_partition_4.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_4.part_name,s_inst->name)){
                        mbr_lectura.mbr_partition_4.part_status = '0';
                        pos_byte = mbr_lectura.mbr_partition_4.part_start;
                        num_bytes = mbr_lectura.mbr_partition_4.part_size;
                    }else{
                        particion_eliminada = false;
                        printf("E: no existe la particion especificada (%s)\n", s_inst->name);
                    }
                    if(particion_eliminada){
                        disco = fopen(s_inst->path, "rb+");
                        char buffer = '\0';
                        for(int i = 0; i < num_bytes; i++){
                            fseek(disco, pos_byte, SEEK_SET);
                            fwrite(&buffer, sizeof(buffer), 1, disco);
                            pos_byte++;
                        }
                        fclose(disco);
                        disco = fopen(s_inst->path, "rb+");
                        fseek(disco, 0, SEEK_SET);
                        fwrite(&mbr_lectura, sizeof(MBR), 1, disco);
                        fclose(disco);
                        printf("Se elimino la particion (%s) en modo Full\n", s_inst->name);
                        /*printf("---Informacion del Disco---\n");
                        printf("Tamanio %d\n", mbr_lectura.mbr_tamanio);
                        printf("-------------Particion 1: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_1.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_1.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_1.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_1.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_1.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_1.part_name);
                        printf("-------------Particion 2: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_2.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_2.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_2.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_2.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_2.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_2.part_name);
                        printf("-------------Particion 3: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_3.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_3.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_3.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_3.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_3.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_3.part_name);
                        printf("-------------Particion 4: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_4.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_4.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_4.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_4.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_4.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_4.part_name);*/
                    }
                }
            }else{
                FILE *disco;
                disco = fopen(s_inst->path, "rb");
                MBR mbr_lectura;
                rewind(disco);
                fread(&mbr_lectura, sizeof(MBR), 1, disco);
                fclose(disco);
                bool tiene_extendida = false;
                int byte_inicio_extendida = 0;
                int tamanio_extendida = 0;
                if(mbr_lectura.mbr_partition_1.part_status == '1'){
                    if(mbr_lectura.mbr_partition_1.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_1.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_1.part_size;
                    }
                }
                if(mbr_lectura.mbr_partition_2.part_status == '1'){
                    if(mbr_lectura.mbr_partition_2.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_2.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_2.part_size;
                    }
                }
                if(mbr_lectura.mbr_partition_3.part_status == '1'){
                    if(mbr_lectura.mbr_partition_3.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_3.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_3.part_size;
                    }
                }
                if(mbr_lectura.mbr_partition_4.part_status == '1'){
                    if(mbr_lectura.mbr_partition_4.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_4.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_4.part_size;
                    }
                }
                if(tiene_extendida){
                    if(existe_part_logica(s_inst->path, byte_inicio_extendida,s_inst->name)){
                        disco = fopen(s_inst->path, "rb");
                        EBR ebr_actual;
                        fseek(disco, byte_inicio_extendida, SEEK_SET);
                        fread(&ebr_actual, sizeof(EBR), 1, disco);
                        fclose(disco);
                        int byte_ebr_anterior = 0;
                        bool esta_en_primer_ebr = true;
                        int formatear_de_byte = 0;
                        int formatear_hasta_byte = 0;
                        do{
                            if(!strcmp(ebr_actual.part_name,s_inst->name)){
                                if(esta_en_primer_ebr){
                                    ebr_actual.part_status = '0';
                                    formatear_de_byte = byte_inicio_extendida + sizeof(EBR);
                                    formatear_hasta_byte = ebr_actual.part_start + (ebr_actual.part_size - 1);
                                    disco = fopen(s_inst->path, "rb+");
                                    fseek(disco, ebr_actual.part_start, SEEK_SET);
                                    fwrite(&ebr_actual, sizeof(EBR), 1, disco);
                                    fclose(disco);
                                }else{
                                    ebr_actual.part_status = '0';
                                    formatear_de_byte = ebr_actual.part_start;
                                    formatear_hasta_byte = ebr_actual.part_start + (ebr_actual.part_size-1);
                                    disco = fopen(s_inst->path, "rb+");
                                    fseek(disco, ebr_actual.part_start, SEEK_SET);
                                    fwrite(&ebr_actual, sizeof(EBR), 1, disco);
                                    fclose(disco);
                                    EBR ebr_anterior;
                                    disco = fopen(s_inst->path, "rb+");
                                    fseek(disco, byte_ebr_anterior, SEEK_SET);
                                    fread(&ebr_anterior, sizeof(EBR), 1, disco);
                                    fclose(disco);
                                    ebr_anterior.part_next = ebr_actual.part_next;
                                    disco = fopen(s_inst->path, "rb+");
                                    fseek(disco, ebr_anterior.part_start, SEEK_SET);
                                    fwrite(&ebr_anterior, sizeof(EBR), 1, disco);
                                    fclose(disco);
                                }
                                break;
                            }
                            esta_en_primer_ebr = false;
                            byte_ebr_anterior = ebr_actual.part_start;
                            if(ebr_actual.part_next != -1){
                                disco = fopen(s_inst->path, "rb");
                                fseek(disco, ebr_actual.part_next, SEEK_SET);
                                fread(&ebr_actual, sizeof(EBR), 1, disco);
                                fclose(disco);
                            }else{
                                break;
                            }
                        }while(true);

                        if(!strncmp(s_inst->delete, "fast",4)){
                            printf("Se eliminó la particion lógica en modo Fast\n");
                        }else{
                            char buffer = '\0';
                            disco = fopen(s_inst->path, "rb+");
                            fseek(disco, formatear_de_byte, SEEK_SET);
                            for(int i = formatear_de_byte; i <= formatear_hasta_byte; i++){
                                fwrite(&buffer,sizeof(buffer),1,disco);
                            }
                            fclose(disco);

                            printf("Se eliminó la partición lógica en modo Full\n");
                            listarEBRs(s_inst->path, byte_inicio_extendida);
                        }
                    }else{
                        printf("E: no existe la partición logica especificada\n");
                    }
                }else{
                    printf("E: No existe una particion extendida a eliminar\n");
                }
            }
        }else if(s_inst->tiene_add){
            if(s_inst->tiene_type == false){
                s_inst->tiene_type = true;
                s_inst->type = 'p';

                bool esLogica = existe_part_logica(s_inst->path, get_inicio_extendida(s_inst->path), s_inst->name);
                if(esLogica){
                    s_inst->type = 'l';
                }
            }
            if(s_inst->type == 'p' ||s_inst->type == 'e'){
                FILE *disco;
                disco = fopen(s_inst->path, "rb");
                MBR mbr_lectura;
                rewind(disco);
                fread(&mbr_lectura, sizeof(MBR), 1, disco);
                bool particion_encontrada = false;
                int num_particion = 0;
                int byte_inicio_part = 0;
                if(!strcmp(mbr_lectura.mbr_partition_1.part_name,s_inst->name)){
                    particion_encontrada = true;
                    num_particion = 1;
                    byte_inicio_part = mbr_lectura.mbr_partition_1.part_start;
                }else if(!strcmp(mbr_lectura.mbr_partition_2.part_name,s_inst->name)){
                    particion_encontrada = true;
                    num_particion = 2;
                    byte_inicio_part = mbr_lectura.mbr_partition_2.part_start;
                }else if(!strcmp(mbr_lectura.mbr_partition_3.part_name,s_inst->name)){
                    particion_encontrada = true;
                    num_particion = 3;
                    byte_inicio_part = mbr_lectura.mbr_partition_3.part_start;
                }else if(!strcmp(mbr_lectura.mbr_partition_4.part_name,s_inst->name)){
                    particion_encontrada = true;
                    num_particion = 4;
                    byte_inicio_part = mbr_lectura.mbr_partition_4.part_start;
                }else{
                    printf("E: no existe la particion especificada (%s)\n", s_inst->name);
                    return;
                }
                int bytes_inicios[4];
                int bytes_tamanios[4];
                if(mbr_lectura.mbr_partition_1.part_status == '1'){
                    bytes_inicios[0] = mbr_lectura.mbr_partition_1.part_start;
                    bytes_tamanios[0] = mbr_lectura.mbr_partition_1.part_size;
                }else{
                    bytes_inicios[0] = -1;
                }
                if(mbr_lectura.mbr_partition_2.part_status == '1'){
                    bytes_inicios[1] = mbr_lectura.mbr_partition_2.part_start;
                    bytes_tamanios[1] = mbr_lectura.mbr_partition_2.part_size;
                }else{
                    bytes_inicios[1] = -1;
                }
                if(mbr_lectura.mbr_partition_3.part_status == '1'){
                    bytes_inicios[2] = mbr_lectura.mbr_partition_3.part_start;
                    bytes_tamanios[2] = mbr_lectura.mbr_partition_3.part_size;
                }else{
                    bytes_inicios[2] = -1;
                }
                if(mbr_lectura.mbr_partition_4.part_status == '1'){
                    bytes_inicios[3] = mbr_lectura.mbr_partition_4.part_start;
                    bytes_tamanios[3] = mbr_lectura.mbr_partition_4.part_size;
                }else{
                    bytes_inicios[3] = -1;
                }
                int n = 4;
                for(int i = 1; i < n; i++){
                    for(int j = 0; j < n-1; j++){
                        if(bytes_inicios[j] > bytes_inicios[j+1]){
                            int aux = bytes_inicios[j];
                            bytes_inicios[j] = bytes_inicios[j+1];
                            bytes_inicios[j+1] = aux;
                            int aux2 = bytes_tamanios[j];
                            bytes_tamanios[j] = bytes_tamanios[j+1];
                            bytes_tamanios[j+1] = aux2;
                        }
                    }
                }
                int espacio_libre = 0;
                for(int i = 0; i < 4; i++){
                    if(bytes_inicios[i] == byte_inicio_part){
                        if(i < 3){
                            espacio_libre = bytes_inicios[i+1] - (bytes_inicios[i]+bytes_tamanios[i]);
                        }else{
                            espacio_libre = mbr_lectura.mbr_tamanio - (bytes_inicios[i]+bytes_tamanios[i]);
                        }
                        break;
                    }
                }
                if(s_inst->add > 0){
                    int agregar_bytes = s_inst->add;
                    if(s_inst->tiene_unit){
                        if(s_inst->unit == 'k'){
                            agregar_bytes = agregar_bytes * 1024;
                        }else if(s_inst->unit == 'm'){
                            agregar_bytes = agregar_bytes * 1024 * 1024;
                        }
                    }else{
                        agregar_bytes = agregar_bytes * 1024;
                    }
                    if(espacio_libre >= agregar_bytes){
                        char nombre_part[16];
                        if(num_particion == 1){
                            mbr_lectura.mbr_partition_1.part_size = mbr_lectura.mbr_partition_1.part_size + agregar_bytes;
                            strcpy(nombre_part,mbr_lectura.mbr_partition_1.part_name);
                        }else if(num_particion == 2){
                            mbr_lectura.mbr_partition_2.part_size = mbr_lectura.mbr_partition_2.part_size + agregar_bytes;
                            strcpy(nombre_part,mbr_lectura.mbr_partition_2.part_name);
                        }else if(num_particion == 3){
                            mbr_lectura.mbr_partition_3.part_size = mbr_lectura.mbr_partition_3.part_size + agregar_bytes;
                            strcpy(nombre_part,mbr_lectura.mbr_partition_3.part_name);
                        }else if(num_particion == 4){
                            mbr_lectura.mbr_partition_4.part_size = mbr_lectura.mbr_partition_4.part_size + agregar_bytes;
                            strcpy(nombre_part,mbr_lectura.mbr_partition_4.part_name);
                        }
                        printf("Espacio agregado a la particion[%s] : %d Bytes\n", nombre_part, agregar_bytes);
                        fclose(disco);
                        disco = fopen(s_inst->path, "rb+");
                        fseek(disco, 0, SEEK_SET);
                        fwrite(&mbr_lectura, sizeof(MBR), 1, disco);
                        fclose(disco);
                        /*printf("---Informacion del Disco---\n");
                        printf("Tamanio %d\n", mbr_lectura.mbr_tamanio);
                        printf("-------------Particion 1: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_1.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_1.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_1.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_1.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_1.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_1.part_name);
                        printf("-------------Particion 2: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_2.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_2.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_2.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_2.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_2.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_2.part_name);
                        printf("-------------Particion 3: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_3.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_3.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_3.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_3.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_3.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_3.part_name);
                        printf("-------------Particion 4: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_4.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_4.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_4.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_4.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_4.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_4.part_name);*/
                    }else{
                        printf("E: no hay espacio libre suficiente continuo a la particion\n");
                        printf("Espacio libre: %d Bytes\n", espacio_libre);
                        return;
                    }
                }else if(s_inst->add < 0){ //Eliminar espacio
                    int agregar_bytes = s_inst->add;
                    if(s_inst->tiene_unit){
                        if(s_inst->unit == 'k'){
                            agregar_bytes = agregar_bytes * 1024;
                        }else if(s_inst->unit == 'm'){
                            agregar_bytes = agregar_bytes * 1024 * 1024;
                        }
                    }else{
                        agregar_bytes = agregar_bytes * 1024;
                    }
                    //printf("Byte a eliminar %d\n", agregar_bytes);
                    bool espacio_eliminado = false;
                    if(num_particion == 1){
                        int result = mbr_lectura.mbr_partition_1.part_size + agregar_bytes;
                        if(result > 0){
                            mbr_lectura.mbr_partition_1.part_size = mbr_lectura.mbr_partition_1.part_size + agregar_bytes;
                            espacio_eliminado = true;
                        }
                    }else if(num_particion == 2){
                        int result = mbr_lectura.mbr_partition_2.part_size + agregar_bytes;
                        if(result > 0){
                            mbr_lectura.mbr_partition_2.part_size = mbr_lectura.mbr_partition_2.part_size + agregar_bytes;
                            espacio_eliminado = true;
                        }
                    }else if(num_particion == 3){
                        int result = mbr_lectura.mbr_partition_3.part_size + agregar_bytes;
                        if(result > 0){
                            mbr_lectura.mbr_partition_3.part_size = mbr_lectura.mbr_partition_3.part_size + agregar_bytes;
                            espacio_eliminado = true;
                        }
                    }else if(num_particion == 4){
                        int result = mbr_lectura.mbr_partition_4.part_size + agregar_bytes;
                        if(result > 0){
                            mbr_lectura.mbr_partition_4.part_size = mbr_lectura.mbr_partition_4.part_size + agregar_bytes;
                            espacio_eliminado = true;
                        }
                    }
                    if(espacio_eliminado){
                        printf("Se eliminó de la particion: %d Bytes\n", -agregar_bytes);

                        disco = fopen(s_inst->path, "rb+");
                        fseek(disco, 0, SEEK_SET);
                        fwrite(&mbr_lectura, sizeof(MBR), 1, disco);
                        fclose(disco);
                        /*printf("---Informacion del Disco---\n");
                        printf("Tamanio %d\n", mbr_lectura.mbr_tamanio);
                        printf("-------------Particion 1: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_1.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_1.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_1.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_1.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_1.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_1.part_name);
                        printf("-------------Particion 2: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_2.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_2.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_2.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_2.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_2.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_2.part_name);
                        printf("-------------Particion 3: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_3.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_3.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_3.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_3.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_3.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_3.part_name);
                        printf("-------------Particion 4: \n");
                        printf("status : %c\n", mbr_lectura.mbr_partition_4.part_status);
                        printf("type : %c\n", mbr_lectura.mbr_partition_4.part_type);
                        printf("fit : %c\n", mbr_lectura.mbr_partition_4.part_fit);
                        printf("start : %d\n", mbr_lectura.mbr_partition_4.part_start);
                        printf("size : %d\n", mbr_lectura.mbr_partition_4.part_size);
                        printf("name : %s\n", mbr_lectura.mbr_partition_4.part_name);*/
                    }else{
                        printf("No fue posible eliminar el espacio del disco, excede su tamaño.\n");
                    }
                }
            }else{
                FILE *disco;
                disco = fopen(s_inst->path, "rb");
                MBR mbr_lectura;
                rewind(disco);
                fread(&mbr_lectura, sizeof(MBR), 1, disco);
                fclose(disco);
                bool tiene_extendida = false;
                int byte_inicio_extendida = 0;
                int tamanio_extendida = 0;
                if(mbr_lectura.mbr_partition_1.part_status == '1'){
                    if(mbr_lectura.mbr_partition_1.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_1.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_1.part_size;
                    }
                }
                if(mbr_lectura.mbr_partition_2.part_status == '1'){
                    if(mbr_lectura.mbr_partition_2.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_2.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_2.part_size;
                    }
                }
                if(mbr_lectura.mbr_partition_3.part_status == '1'){
                    if(mbr_lectura.mbr_partition_3.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_3.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_3.part_size;
                    }
                }
                if(mbr_lectura.mbr_partition_4.part_status == '1'){
                    if(mbr_lectura.mbr_partition_4.part_type == 'e'){
                        tiene_extendida = true;
                        byte_inicio_extendida = mbr_lectura.mbr_partition_4.part_start;
                        tamanio_extendida = mbr_lectura.mbr_partition_4.part_size;
                    }
                }
                if(tiene_extendida){
                    if(existe_part_logica(s_inst->path, byte_inicio_extendida,s_inst->name)){
                        disco = fopen(s_inst->path, "rb");
                        EBR ebr_actual;
                        fseek(disco, byte_inicio_extendida, SEEK_SET);
                        fread(&ebr_actual, sizeof(EBR), 1, disco);
                        fclose(disco);
                        int espacio_libre = 0;
                        do{
                            if(!strcmp(ebr_actual.part_name,s_inst->name)){

                                if(ebr_actual.part_next == -1){
                                    espacio_libre = tamanio_extendida - (ebr_actual.part_start+ebr_actual.part_size);
                                }else{
                                    espacio_libre = ebr_actual.part_next - (ebr_actual.part_start+ebr_actual.part_size);
                                }
                                int agregar_bytes = s_inst->add;
                                if(s_inst->tiene_unit){
                                    if(s_inst->unit == 'k'){
                                        agregar_bytes = agregar_bytes * 1024;
                                    }else if(s_inst->unit == 'm'){
                                        agregar_bytes = agregar_bytes * 1024 * 1024;
                                    }
                                }else{
                                    agregar_bytes = agregar_bytes * 1024;
                                }
                                printf("Bytes a agregar %d\n", agregar_bytes);
                                if(agregar_bytes > 0){
                                    if(espacio_libre >= agregar_bytes){
                                        ebr_actual.part_size = ebr_actual.part_size + agregar_bytes;
                                    }else{
                                        printf("E: No hay suficiente espacio libre\n");
                                        printf("Espacio libre: %d\n", espacio_libre);
                                        return;
                                    }
                                }else if(agregar_bytes < 0){
                                    int result = ebr_actual.part_size + agregar_bytes;
                                    if(result > sizeof(EBR)){
                                        ebr_actual.part_size = ebr_actual.part_size + agregar_bytes;
                                    }else{
                                        printf("E: No quedará espacio para el EBR de esta particion\n");
                                        return;
                                    }
                                }
                                disco = fopen(s_inst->path, "rb+");
                                fseek(disco, ebr_actual.part_start, SEEK_SET);
                                fwrite(&ebr_actual, sizeof(EBR), 1, disco);
                                fclose(disco);
                                printf("Particion lógica modificada\n");
                                listarEBRs(s_inst->path, byte_inicio_extendida);
                                break;
                            }
                            if(ebr_actual.part_next != -1){
                                disco = fopen(s_inst->path, "rb");
                                fseek(disco, ebr_actual.part_next, SEEK_SET);
                                fread(&ebr_actual, sizeof(EBR), 1, disco);
                                fclose(disco);
                            }else{
                                break;
                            }
                        }while(true);
                    }else{
                        printf("E: no existe la particíon logica especificada\n");
                    }
                }else{
                    printf("E: no existe una particion extendida a eliminar\n");
                }
            }
        }else{
            printf("E: faltan parámetros obligatorios '-size'|'-name'|'-path'| \n");
        }
    }else if(!strcmp(s_inst->comando, "mount")){
        if(access(s_inst->path, F_OK) == -1){
            printf("E: no existe la ruta o archivo especificado\n");
            return;
        }
        FILE *disco;
        disco = fopen(s_inst->path, "rb");
        MBR mbr_lectura;
        rewind(disco);
        fread(&mbr_lectura, sizeof(MBR), 1, disco);
        fclose(disco);
        bool existe_particion = false;
        bool existe_particion_logica = false;
        if(mbr_lectura.mbr_partition_1.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_1.part_name,s_inst->name)){
            existe_particion = true;
        }else if(mbr_lectura.mbr_partition_2.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_2.part_name,s_inst->name)){
            existe_particion = true;
        }else if(mbr_lectura.mbr_partition_3.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_3.part_name,s_inst->name)){
            existe_particion = true;
        }else if(mbr_lectura.mbr_partition_4.part_status == '1' && !strcmp(mbr_lectura.mbr_partition_4.part_name,s_inst->name)){
            existe_particion = true;
        }
        if(existe_particion == false){
            FILE *disco;
            disco = fopen(s_inst->path, "rb");
            MBR mbr_lectura;
            rewind(disco);
            fread(&mbr_lectura, sizeof(MBR), 1, disco);
            fclose(disco);
            bool tiene_extendida = false;
            int byte_inicio_extendida = 0;
            int tamanio_extendida = 0;
            if(mbr_lectura.mbr_partition_1.part_status == '1'){
                if(mbr_lectura.mbr_partition_1.part_type == 'e'){
                    tiene_extendida = true;
                    byte_inicio_extendida = mbr_lectura.mbr_partition_1.part_start;
                    tamanio_extendida = mbr_lectura.mbr_partition_1.part_size;
                }
            }
            if(mbr_lectura.mbr_partition_2.part_status == '1'){
                if(mbr_lectura.mbr_partition_2.part_type == 'e'){
                    tiene_extendida = true;
                    byte_inicio_extendida = mbr_lectura.mbr_partition_2.part_start;
                    tamanio_extendida = mbr_lectura.mbr_partition_2.part_size;
                }
            }
            if(mbr_lectura.mbr_partition_3.part_status == '1'){
                if(mbr_lectura.mbr_partition_3.part_type == 'e'){
                    tiene_extendida = true;
                    byte_inicio_extendida = mbr_lectura.mbr_partition_3.part_start;
                    tamanio_extendida = mbr_lectura.mbr_partition_3.part_size;
                }
            }
            if(mbr_lectura.mbr_partition_4.part_status == '1'){
                if(mbr_lectura.mbr_partition_4.part_type == 'e'){
                    tiene_extendida = true;
                    byte_inicio_extendida = mbr_lectura.mbr_partition_4.part_start;
                    tamanio_extendida = mbr_lectura.mbr_partition_4.part_size;
                }
            }
            if(tiene_extendida){
                if(existe_part_logica(s_inst->path, byte_inicio_extendida,s_inst->name)){
                    existe_particion_logica = true;
                }
            }
        }
        if(existe_particion || existe_particion_logica){
            if(existeDisco(s_inst->path) && existeParticion(s_inst->name)){
                printf("W: La particion ya ha sido montada\n");
            }else{
                char ldisco = 'x';
                int npart = 0;
                if(!existeDisco(s_inst->path)){
                    agregarDisco(s_inst->path);
                }
                ldisco = get_letra_disco(s_inst->path);
                agregarParticion(s_inst->name, ldisco);
                npart = get_numero_particion(s_inst->name, ldisco);
                printf("\nParticion montada ID = vd%c%d\n",ldisco,npart);
            }
        }else{
            printf("E: no existe la particion especificada\n");
        }
    }else if(!strcmp(s_inst->comando, "umount")){
       for(int i = 0; i < 10; i++){
           char* id = getitem(parts, i);
           if(NULL != id){
               printf("El montaje es %s\n", id);
               if(existeMontaje(id)){
                   printf("Existe el montaje\n");
                   printf("Path: %s\n", get_path(id));
                   desmontar(id);
               }else{
                   printf("E: partición %s no se encuentra montada.\n", id);
               }
           }
       }
       destroy(&parts);
    }else if(!strcmp(s_inst->comando, "rep")){
        if(existeMontaje(s_inst->id)){

            char path_backup[256];
            strcpy(path_backup, s_inst->path);

            char aux_path[256];
            reiniciar_path(aux_path);
            aux_path[0] = '/';

            char *tkn;
            tkn = strtok(path_backup, "/");
            int conteoTokens = 0;
            while(tkn != NULL){
                conteoTokens++;
                tkn = strtok(NULL, "/");
            }
            reiniciar_path(aux_path);
            reiniciar_path(path_backup);
            strcpy(path_backup, s_inst->path);
            aux_path[0] = '/';
            char *tkn2;
            tkn2 = strtok(path_backup, "/");
            while(tkn2 != NULL && conteoTokens > 1){
                strcat(aux_path, tkn2);
                if(access(aux_path, F_OK) == -1){
                    mkdir(aux_path, 0700);
                    strcat(aux_path,"/");
                }else{
                    strcat(aux_path,"/");
                }
                tkn2 = strtok(NULL, "/");
                conteoTokens--;
            }
            if(!strncmp(s_inst->name, "mbr",3)){
                FILE *disco;
                disco = fopen(get_path(s_inst->id), "rb");
                MBR mbr_lectura;
                rewind(disco);
                fread(&mbr_lectura, sizeof(MBR), 1, disco);
                fclose(disco);
                char path_temporal[256];
                reiniciar_token(path_temporal);
                strncpy(path_temporal, s_inst->path, strlen(s_inst->path)-4);
                strcat(path_temporal, ".txt");
                //printf("La ruta temp del archivo MBR es %s y el path es %s ", path_temporal, s_inst->path);
                FILE *f = fopen(path_temporal, "wb+");
                //fprintf(f, "Some text: %s\n", text);
                fprintf(f, "digraph reporte {\n");
                fprintf(f, "node [shape=plaintext]\n");

                fprintf(f, "mbr [label=<\n");
                fprintf(f, "<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n");

                fprintf(f, "<tr><td>Nombre</td><td>Valor</td></tr>\n");
                fprintf(f, "<tr><td>mbr_tamanio</td><td>%d</td></tr>\n",mbr_lectura.mbr_tamanio);
                fprintf(f, "<tr><td>mbr_fecha_creacion</td><td>%s</td></tr>\n",asctime(localtime(&mbr_lectura.mbr_fecha_creacion)));
                fprintf(f, "<tr><td>mbr_disk_signature</td><td>%d</td></tr>\n",mbr_lectura.mbr_disk_signature);

                if(mbr_lectura.mbr_partition_1.part_status == '1'){
                    fprintf(f, "<tr><td>part_status_1</td><td>%c</td></tr>\n",mbr_lectura.mbr_partition_1.part_status);
                    fprintf(f, "<tr><td>part_type_1</td><td>%c</td></tr>\n",mbr_lectura.mbr_partition_1.part_type);
                    fprintf(f, "<tr><td>part_fit_1</td><td>%c</td></tr>\n",mbr_lectura.mbr_partition_1.part_fit);
                    fprintf(f, "<tr><td>part_start_1</td><td>%d</td></tr>\n",mbr_lectura.mbr_partition_1.part_start);
                    fprintf(f, "<tr><td>part_size_1</td><td>%d</td></tr>\n",mbr_lectura.mbr_partition_1.part_size);
                    fprintf(f, "<tr><td>part_name_1</td><td>%s</td></tr>\n",mbr_lectura.mbr_partition_1.part_name);
                }


                if(mbr_lectura.mbr_partition_2.part_status == '1'){
                    fprintf(f, "<tr><td>part_status_2</td><td>%c</td></tr>",mbr_lectura.mbr_partition_2.part_status);
                    fprintf(f, "<tr><td>part_type_2</td><td>%c</td></tr>",mbr_lectura.mbr_partition_2.part_type);
                    fprintf(f, "<tr><td>part_fit_2</td><td>%c</td></tr>",mbr_lectura.mbr_partition_2.part_fit);
                    fprintf(f, "<tr><td>part_start_2</td><td>%d</td></tr>",mbr_lectura.mbr_partition_2.part_start);
                    fprintf(f, "<tr><td>part_size_2</td><td>%d</td></tr>",mbr_lectura.mbr_partition_2.part_size);
                    fprintf(f, "<tr><td>part_name_2</td><td>%s</td></tr>",mbr_lectura.mbr_partition_2.part_name);
                }

                if(mbr_lectura.mbr_partition_3.part_status == '1'){
                    fprintf(f, "<tr><td>part_status_3</td><td>%c</td></tr>",mbr_lectura.mbr_partition_3.part_status);
                    fprintf(f, "<tr><td>part_type_3</td><td>%c</td></tr>",mbr_lectura.mbr_partition_3.part_type);
                    fprintf(f, "<tr><td>part_fit_3</td><td>%c</td></tr>",mbr_lectura.mbr_partition_3.part_fit);
                    fprintf(f, "<tr><td>part_start_3</td><td>%d</td></tr>",mbr_lectura.mbr_partition_3.part_start);
                    fprintf(f, "<tr><td>part_size_3</td><td>%d</td></tr>",mbr_lectura.mbr_partition_3.part_size);
                    fprintf(f, "<tr><td>part_name_3</td><td>%s</td></tr>",mbr_lectura.mbr_partition_3.part_name);
                }
                if(mbr_lectura.mbr_partition_4.part_status == '1'){
                    fprintf(f, "<tr><td>part_status_4</td><td>%c</td></tr>",mbr_lectura.mbr_partition_4.part_status);
                    fprintf(f, "<tr><td>part_type_4</td><td>%c</td></tr>",mbr_lectura.mbr_partition_4.part_type);
                    fprintf(f, "<tr><td>part_fit_4</td><td>%c</td></tr>",mbr_lectura.mbr_partition_4.part_fit);
                    fprintf(f, "<tr><td>part_start_4</td><td>%d</td></tr>",mbr_lectura.mbr_partition_4.part_start);
                    fprintf(f, "<tr><td>part_size_4</td><td>%d</td></tr>",mbr_lectura.mbr_partition_4.part_size);
                    fprintf(f, "<tr><td>part_name_4</td><td>%s</td></tr>",mbr_lectura.mbr_partition_4.part_name);
                }
                fprintf(f, "</table>\n");
                fprintf(f, ">];\n");
                char path_id_montado[256];
                //path_id_montado = get_path(s_inst->id);
                strcpy(path_id_montado, get_path(s_inst->id));
                //printf("Path montada: %s\n", path_id_montado);
                int byte_inicio_extendida = get_inicio_extendida(path_id_montado);
                //printf("Byte inicio Extendida: %d\n", byte_inicio_extendida);
                if(byte_inicio_extendida != -1){
                    //Leer el EBR
                    disco = fopen(path_id_montado, "rb");
                    EBR ebr_actual;
                    fseek(disco, byte_inicio_extendida, SEEK_SET);
                    fread(&ebr_actual, sizeof(EBR), 1, disco);
                    fclose(disco);
                    int contador_ebr = 1;

                    do{
                        /*printf("Nombre: %s\n", ebr_actual.part_name);
                        printf("Status: %c\n", ebr_actual.part_status);
                        printf("Fit: %c\n", ebr_actual.part_fit);
                        printf("Start: %d\n", ebr_actual.part_start);
                        printf("Size: %d\n", ebr_actual.part_size);
                        printf("Next: %d\n", ebr_actual.part_next);*/
                        fprintf(f, "ebr%d [label=<\n",contador_ebr);
                        fprintf(f, "<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n");
                        fprintf(f, "<tr><td>Nombre</td><td>Valor</td></tr>");
                        fprintf(f, "<tr><td>part_status</td><td>%c</td></tr>",ebr_actual.part_status);
                        fprintf(f, "<tr><td>part_fit</td><td>%c</td></tr>",ebr_actual.part_fit);
                        fprintf(f, "<tr><td>part_start</td><td>%d</td></tr>",ebr_actual.part_start);
                        fprintf(f, "<tr><td>part_size</td><td>%d</td></tr>",ebr_actual.part_size);
                        fprintf(f, "<tr><td>part_next</td><td>%d</td></tr>",ebr_actual.part_next);
                        fprintf(f, "<tr><td>part_name</td><td>%s</td></tr>",ebr_actual.part_name);
                        fprintf(f, "</table>\n");
                        fprintf(f, ">];\n");
                        contador_ebr++;
                        if(ebr_actual.part_next != -1){
                            disco = fopen(path_id_montado, "rb");
                            fseek(disco, ebr_actual.part_next, SEEK_SET);
                            fread(&ebr_actual, sizeof(EBR), 1, disco);
                            fclose(disco);
                        }else{
                            break;
                        }
                    }while(true);
                }
                fprintf(f, "}\n");
                fclose(f);
                char comando_reporte[512];
                strcpy(comando_reporte, "dot -Tjpg ");
                strcat(comando_reporte, path_temporal);
                strcat(comando_reporte, " > ");
                strcat(comando_reporte, s_inst->path);
                system(comando_reporte);
                printf("Se creo el reporte: %s\n", s_inst->path);
            }else if(!strncmp(s_inst->name, "disk",4)){
                FILE *disco;
                disco = fopen(get_path(s_inst->id), "rb");
                MBR mbr_lectura;
                rewind(disco);
                fread(&mbr_lectura, sizeof(MBR), 1, disco);
                fclose(disco);
                char path_temporal[256];
                reiniciar_token(path_temporal);
                strncpy(path_temporal, s_inst->path, strlen(s_inst->path)-4);
                strcat(path_temporal, ".txt");
                //printf("La ruta temp del archivo DISK es %s y el path es %s ", path_temporal, s_inst->path);
                FILE *f = fopen(path_temporal, "wb+");
                fprintf(f, "digraph reporte {\n");
                fprintf(f, "node [shape=record]\n");
                fprintf(f, "disk [shape=record,label=\"\n");
                fprintf(f, "MBR");
                int bytes_inicios[4] = {0};
                int bytes_tamanios[4] = {0};
                char tipos_particiones[4][10];
                int numeros_partition[4] = {1,2,3,4};
                if(mbr_lectura.mbr_partition_1.part_status == '1'){
                    bytes_inicios[0] = mbr_lectura.mbr_partition_1.part_start;
                    bytes_tamanios[0] = mbr_lectura.mbr_partition_1.part_size;
                    if(mbr_lectura.mbr_partition_1.part_type == 'p'){
                        strcpy(tipos_particiones[0], "primaria");
                    }else{
                        strcpy(tipos_particiones[0], "extendida");
                    }
                }
                if(mbr_lectura.mbr_partition_2.part_status == '1'){
                    bytes_inicios[1] = mbr_lectura.mbr_partition_2.part_start;
                    bytes_tamanios[1] = mbr_lectura.mbr_partition_2.part_size;
                    if(mbr_lectura.mbr_partition_2.part_type == 'p'){
                        strcpy(tipos_particiones[1], "primaria");
                    }else{
                        strcpy(tipos_particiones[1], "extendida");
                    }
                }
                if(mbr_lectura.mbr_partition_3.part_status == '1'){
                    bytes_inicios[2] = mbr_lectura.mbr_partition_3.part_start;
                    bytes_tamanios[2] = mbr_lectura.mbr_partition_3.part_size;
                    if(mbr_lectura.mbr_partition_3.part_type == 'p'){
                        strcpy(tipos_particiones[2], "primaria");
                    }else{
                        strcpy(tipos_particiones[2], "extendida");
                    }
                }
                if(mbr_lectura.mbr_partition_4.part_status == '1'){
                    bytes_inicios[3] = mbr_lectura.mbr_partition_4.part_start;
                    bytes_tamanios[3] = mbr_lectura.mbr_partition_4.part_size;
                    if(mbr_lectura.mbr_partition_4.part_type == 'p'){
                        strcpy(tipos_particiones[3], "primaria");
                    }else{
                        strcpy(tipos_particiones[3], "extendida");
                    }
                }
                int n = 4;
                for (int i = 1; i < n; i++){
                    for (int j = 0; j < n-1; j++){
                        if(bytes_inicios[j] > bytes_inicios[j+1]){
                            int aux = bytes_inicios[j];
                            bytes_inicios[j] = bytes_inicios[j+1];
                            bytes_inicios[j+1] = aux;

                            int aux2 = bytes_tamanios[j];
                            bytes_tamanios[j] = bytes_tamanios[j+1];
                            bytes_tamanios[j+1] = aux2;

                            char auxTipo[10] = {'\0'};
                            strcpy(auxTipo, tipos_particiones[j]);
                            strcpy(tipos_particiones[j], tipos_particiones[j+1]);
                            strcpy(tipos_particiones[j+1], auxTipo);

                            int aux3 = numeros_partition[j];
                            numeros_partition[j] = numeros_partition[j+1];
                            numeros_partition[j+1] = aux3;
                        }
                    }
                }
                bool primer_particion_encontrada = false;
                for (int i = 0; i < n; ++i){
                    //printf("bytes_inicios[%d] : %d\n",i, bytes_inicios[i]);
                    if(bytes_inicios[i] != 0 && !primer_particion_encontrada){
                        primer_particion_encontrada = true;
                        if((sizeof(MBR) - bytes_inicios[i]) == 0){
                            if(!strcmp(tipos_particiones[i], "primaria")){
                                fprintf(f, "|Particion %d (Primaria)", numeros_partition[i]);
                            }else{
                                fprintf(f, "|{Particion %d (Extendida)", numeros_partition[i]);
                                int num_logicas = cantidad_particiones_logicas(get_path(s_inst->id));
                                bool tiene_logicas = false;
                                if(num_logicas > 0){
                                    fprintf(f, "|{ EBR| Logica");
                                    tiene_logicas = true;
                                    num_logicas--;
                                }
                                while(num_logicas > 0){
                                    fprintf(f, "| EBR| Logica");
                                    num_logicas--;
                                }
                                if(tiene_logicas){
                                    fprintf(f,"}");
                                }
                                fprintf(f,"}");
                            }
                        }else{
                            if(!strcmp(tipos_particiones[i], "primaria")){
                                fprintf(f, "|Espacio Libre|Particion %d (Primaria)", numeros_partition[i]);
                            }else{
                                fprintf(f, "|Espacio Libre|{Particion %d (Extendida)", numeros_partition[i]);
                                int num_logicas = cantidad_particiones_logicas(get_path(s_inst->id));
                                bool tiene_logicas = false;
                                if(num_logicas > 0){
                                    fprintf(f, "|{ EBR| Logica");
                                    tiene_logicas = true;
                                    num_logicas--;
                                }
                                while(num_logicas > 0){
                                    fprintf(f, "| EBR| Logica");
                                    num_logicas--;
                                }
                                if(tiene_logicas){
                                    fprintf(f,"}");
                                }
                                fprintf(f,"}");                           }
                        }
                        if(i == 3){
                            if((mbr_lectura.mbr_tamanio - (bytes_inicios[i] + bytes_tamanios[i])) != 0){
                                fprintf(f, "|Espacio Libre");
                            }
                        }
                        continue;
                    }
                    if(bytes_inicios[i] != 0 && primer_particion_encontrada){
                        //printf("I : %d\n", i);
                        //printf("bytes_inicios[i] - (bytes_inicios[i-1] + bytes_tamanios[i-1])) = %d\n", bytes_inicios[i] - (bytes_inicios[i-1] + bytes_tamanios[i-1]));
                        if(bytes_inicios[i-1] != 0 && ((bytes_inicios[i] - (bytes_inicios[i-1] + bytes_tamanios[i-1])) != 0)){
                            if(!strcmp(tipos_particiones[i], "primaria")){
                                fprintf(f, "|Espacio Libre|Particion %d (Primaria)", numeros_partition[i]);
                            }else{
                                fprintf(f, "|Espacio Libre|{Particion %d (Extendida)", numeros_partition[i]);
                                int num_logicas = cantidad_particiones_logicas(get_path(s_inst->id));
                                bool tiene_logicas = false;
                                if(num_logicas > 0){
                                    fprintf(f, "|{ EBR| Logica");
                                    tiene_logicas = true;
                                    num_logicas--;
                                }
                                while(num_logicas > 0){
                                    fprintf(f, "| EBR| Logica");
                                    num_logicas--;
                                }
                                if(tiene_logicas){
                                    fprintf(f,"}");
                                }
                                fprintf(f,"}");
                            }
                        }else{
                            if(!strcmp(tipos_particiones[i], "primaria")){
                                fprintf(f, "|Particion %d (Primaria)", numeros_partition[i]);
                            }else{
                                fprintf(f, "|{Particion %d (Extendida)", numeros_partition[i]);
                                int num_logicas = cantidad_particiones_logicas(get_path(s_inst->id));
                                bool tiene_logicas = false;
                                if(num_logicas > 0){
                                    fprintf(f, "|{ EBR| Logica");
                                    tiene_logicas = true;
                                    num_logicas--;
                                }
                                while(num_logicas > 0){
                                    fprintf(f, "| EBR| Logica");
                                    num_logicas--;
                                }
                                if(tiene_logicas){
                                    fprintf(f,"}");
                                }
                                fprintf(f,"}");
                            }
                        }
                        if(i == 3){
                            if((mbr_lectura.mbr_tamanio - (bytes_inicios[i] + bytes_tamanios[i])) != 0){
                                fprintf(f, "|Espacio Libre");
                            }
                        }
                    }
                }
                fprintf(f, "\"];\n");
                fprintf(f, "}\n");
                fclose(f);
                char comando_reporte[512];
                strcpy(comando_reporte, "dot -Tjpg ");
                strcat(comando_reporte, path_temporal);
                strcat(comando_reporte, " > ");
                strcat(comando_reporte, s_inst->path);
                //printf("%s\n", comando_reporte);
                system(comando_reporte);
                printf("Se creo el reporte: %s\n", s_inst->path);
            }else{
                printf("E: parámetro no acepta valor desconocido '%s'\n", s_inst->name);
            }
        }else{
            printf("E: La partición %s no se encuentra montada\n", s_inst->id);
        }
    }
}
MBR crearMBR(int num_bytes){
    MBR mbr;
    mbr.mbr_tamanio = num_bytes;
    mbr.mbr_disk_signature = rand();
    mbr.mbr_fecha_creacion = time(NULL);
    mbr.mbr_partition_1.part_status = '0';
    mbr.mbr_partition_2.part_status = '0';
    mbr.mbr_partition_3.part_status = '0';
    mbr.mbr_partition_4.part_status = '0';

    mbr.mbr_partition_1.part_type = 'p';
    mbr.mbr_partition_2.part_type = 'p';
    mbr.mbr_partition_3.part_type = 'p';
    mbr.mbr_partition_4.part_type = 'p';

    mbr.mbr_partition_1.part_fit = 'f';
    mbr.mbr_partition_2.part_fit = 'f';
    mbr.mbr_partition_3.part_fit = 'f';
    mbr.mbr_partition_4.part_fit = 'f';

    mbr.mbr_partition_1.part_start = 0;
    mbr.mbr_partition_2.part_start = 0;
    mbr.mbr_partition_3.part_start = 0;
    mbr.mbr_partition_4.part_start = 0;

    mbr.mbr_partition_1.part_size = 0;
    mbr.mbr_partition_2.part_size = 0;
    mbr.mbr_partition_3.part_size = 0;
    mbr.mbr_partition_4.part_size = 0;
    return mbr;
}

EBR crearEBR(){
    EBR ebr;
    ebr.part_status = '1';
    ebr.part_fit = 'f';
    ebr.part_start = 0;
    ebr.part_size = 0;
    ebr.part_next = -1;
    return ebr;
}

void listarEBRs(char *path, int inicia_ebr){
    FILE *disco;
    disco = fopen(path, "rb");
    EBR ebr_actual;
    fseek(disco, inicia_ebr, SEEK_SET);
    fread(&ebr_actual, sizeof(EBR), 1, disco);
    fclose(disco);
    do{
        /*printf("---EBR:\n");
        printf("Nombre: %s\n", ebr_actual.part_name);
        printf("Status: %c\n", ebr_actual.part_status);
        printf("Fit: %c\n", ebr_actual.part_fit);
        printf("Start: %d\n", ebr_actual.part_start);
        printf("Size: %d\n", ebr_actual.part_size);
        printf("Next: %d\n", ebr_actual.part_next);*/
        if(ebr_actual.part_next != -1){
            disco = fopen(path, "rb");
            fseek(disco, ebr_actual.part_next, SEEK_SET);
            fread(&ebr_actual, sizeof(EBR), 1, disco);
            fclose(disco);
        }else{
            break;
        }
    }while(true);
}

bool existe_part_logica(char *path, int inicia_ebr, char *nombre){
    FILE *disco;
    disco = fopen(path, "rb");
    EBR ebr_actual;
    fseek(disco, inicia_ebr, SEEK_SET);
    fread(&ebr_actual, sizeof(EBR), 1, disco);
    fclose(disco);
    bool existe_ebr = false;
    do{
        if((!strcmp(ebr_actual.part_name,nombre)) && ebr_actual.part_status == '1'){
            existe_ebr = true;
        }

        if(ebr_actual.part_next != -1){
            disco = fopen(path, "rb");
            fseek(disco, ebr_actual.part_next, SEEK_SET);
            fread(&ebr_actual, sizeof(EBR), 1, disco);
            fclose(disco);
        }else{
            break;
        }
    }while(true);

    return existe_ebr;
}

EBR get_ebr(char *path, int inicia_ebr, char *nombre){
    FILE *disco;
    disco = fopen(path, "rb");
    EBR ebr_actual;
    fseek(disco, inicia_ebr, SEEK_SET);
    fread(&ebr_actual, sizeof(EBR), 1, disco);
    fclose(disco);
    bool existe_ebr = false;
    do{
        if((!strcmp(ebr_actual.part_name,nombre)) && ebr_actual.part_status == '1'){
            return ebr_actual;
        }

        if(ebr_actual.part_next != -1){
            disco = fopen(path, "rb");
            fseek(disco, ebr_actual.part_next, SEEK_SET);
            fread(&ebr_actual, sizeof(EBR), 1, disco);
            fclose(disco);
        }else{
            break;
        }
    }while(true);
}

int get_inicio_extendida(char *path){
    FILE *disco;
    disco = fopen(path, "rb");
    MBR mbr_lectura;
    rewind(disco);
    fread(&mbr_lectura, sizeof(MBR), 1, disco);
    fclose(disco);
    bool tiene_extendida = false;
    int byte_inicio_extendida = 0;
    int tamanio_extendida = 0;
    if(mbr_lectura.mbr_partition_1.part_status == '1'){
        if(mbr_lectura.mbr_partition_1.part_type == 'e'){
            tiene_extendida = true;
            byte_inicio_extendida = mbr_lectura.mbr_partition_1.part_start;
            tamanio_extendida = mbr_lectura.mbr_partition_1.part_size;
        }
    }
    if(mbr_lectura.mbr_partition_2.part_status == '1'){
        if(mbr_lectura.mbr_partition_2.part_type == 'e'){
            tiene_extendida = true;
            byte_inicio_extendida = mbr_lectura.mbr_partition_2.part_start;
            tamanio_extendida = mbr_lectura.mbr_partition_2.part_size;
        }
    }
    if(mbr_lectura.mbr_partition_3.part_status == '1'){
        if(mbr_lectura.mbr_partition_3.part_type == 'e'){
            tiene_extendida = true;
            byte_inicio_extendida = mbr_lectura.mbr_partition_3.part_start;
            tamanio_extendida = mbr_lectura.mbr_partition_3.part_size;
        }
    }
    if(mbr_lectura.mbr_partition_4.part_status == '1'){
        if(mbr_lectura.mbr_partition_4.part_type == 'e'){
            tiene_extendida = true;
            byte_inicio_extendida = mbr_lectura.mbr_partition_4.part_start;
            tamanio_extendida = mbr_lectura.mbr_partition_4.part_size;
        }
    }
    if(tiene_extendida){
        return byte_inicio_extendida;
    }else{
        return -1;
    }
}
int cantidad_particiones_logicas(char *path){
    int cantidad_particiones = 0;
    FILE *disco;
    disco = fopen(path, "rb");
    EBR ebr_actual;
    fseek(disco, get_inicio_extendida(path), SEEK_SET);
    fread(&ebr_actual, sizeof(EBR), 1, disco);
    fclose(disco);
    do{
        /*printf("---EBR:\n");
        printf("Nombre: %s\n", ebr_actual.part_name);
        printf("Status: %c\n", ebr_actual.part_status);
        printf("Fit: %c\n", ebr_actual.part_fit);
        printf("Start: %d\n", ebr_actual.part_start);
        printf("Size: %d\n", ebr_actual.part_size);
        printf("Next: %d\n", ebr_actual.part_next);*/

        if(ebr_actual.part_status == '1'){
            cantidad_particiones++;
        }
        if(ebr_actual.part_next != -1){
            disco = fopen(path, "rb");
            fseek(disco, ebr_actual.part_next, SEEK_SET);
            fread(&ebr_actual, sizeof(EBR), 1, disco);
            fclose(disco);
        }else{
            break;
        }
    }while(true);

    return cantidad_particiones;
}
char get_tipo_particion(char *path_disco, char *nombre){
    if(existe_part_logica(path_disco, get_inicio_extendida(path_disco), nombre)){
        return 'l';
    }else{
        FILE *disco;
        disco = fopen(path_disco, "rb");
        MBR mbr_lectura;
        rewind(disco);
        fread(&mbr_lectura, sizeof(MBR), 1, disco);
        fclose(disco);

        if(mbr_lectura.mbr_partition_1.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_1.part_name)){
            if(mbr_lectura.mbr_partition_1.part_type == 'p'){
                return 'p';
            }else{
                return 'e';
            }
        }else if(mbr_lectura.mbr_partition_2.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_2.part_name)){
            if(mbr_lectura.mbr_partition_2.part_type == 'p'){
                return 'p';
            }else{
                return 'e';
            }
        }else if(mbr_lectura.mbr_partition_3.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_3.part_name)){
            if(mbr_lectura.mbr_partition_3.part_type == 'p'){
                return 'p';
            }else{
                return 'e';
            }
        }else if(mbr_lectura.mbr_partition_4.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_4.part_name)){
            if(mbr_lectura.mbr_partition_4.part_type == 'p'){
                return 'p';
            }else{
                return 'e';
            }
        }else{
            return 'n';
        }
    }
}
int get_inicio_particion(char *path_disco, char tipo_particion, char *nombre){
    if(tipo_particion == 'l'){
        EBR ebr_busqueda = get_ebr(path_disco, get_inicio_extendida(path_disco), nombre);
        return ebr_busqueda.part_start;
    }else{
        FILE *disco;
        disco = fopen(path_disco, "rb");
        MBR mbr_lectura;
        rewind(disco);
        fread(&mbr_lectura, sizeof(MBR), 1, disco);
        fclose(disco);

        if(mbr_lectura.mbr_partition_1.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_1.part_name)){
            return mbr_lectura.mbr_partition_1.part_start;
        }else if(mbr_lectura.mbr_partition_2.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_2.part_name)){
            return mbr_lectura.mbr_partition_2.part_start;
        }else if(mbr_lectura.mbr_partition_3.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_3.part_name)){
            return mbr_lectura.mbr_partition_3.part_start;
        }else if(mbr_lectura.mbr_partition_4.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_4.part_name)){
            return mbr_lectura.mbr_partition_4.part_start;
        }else{
            return -1;
        }
    }
}
char get_ajuste_particion(char *path_disco, char tipo_particion, char *nombre){
    if(tipo_particion == 'l'){
        EBR ebr_busqueda = get_ebr(path_disco, get_inicio_extendida(path_disco), nombre);
        return ebr_busqueda.part_fit;
    }else{
        FILE *disco;
        disco = fopen(path_disco, "rb");
        MBR mbr_lectura;
        rewind(disco);
        fread(&mbr_lectura, sizeof(MBR), 1, disco);
        fclose(disco);

        if(mbr_lectura.mbr_partition_1.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_1.part_name)){
            return mbr_lectura.mbr_partition_1.part_fit;
        }else if(mbr_lectura.mbr_partition_2.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_2.part_name)){
            return mbr_lectura.mbr_partition_2.part_fit;
        }else if(mbr_lectura.mbr_partition_3.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_3.part_name)){
            return mbr_lectura.mbr_partition_3.part_fit;
        }else if(mbr_lectura.mbr_partition_4.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_4.part_name)){
            return mbr_lectura.mbr_partition_4.part_fit;
        }else{
            return -1;
        }
    }
}
int get_size_particion(char *path_disco, char tipo_particion, char *nombre){
    if(tipo_particion == 'l'){
        EBR ebr_busqueda = get_ebr(path_disco, get_inicio_extendida(path_disco), nombre);
        return ebr_busqueda.part_size;
    }else{
        FILE *disco;
        disco = fopen(path_disco, "rb");
        MBR mbr_lectura;
        rewind(disco);
        fread(&mbr_lectura, sizeof(MBR), 1, disco);
        fclose(disco);

        if(mbr_lectura.mbr_partition_1.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_1.part_name)){
            return mbr_lectura.mbr_partition_1.part_size;
        }else if(mbr_lectura.mbr_partition_2.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_2.part_name)){
            return mbr_lectura.mbr_partition_2.part_size;
        }else if(mbr_lectura.mbr_partition_3.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_3.part_name)){
            return mbr_lectura.mbr_partition_3.part_size;
        }else if(mbr_lectura.mbr_partition_4.part_status == '1' && !strcmp(nombre, mbr_lectura.mbr_partition_4.part_name)){
            return mbr_lectura.mbr_partition_4.part_size;
        }else{
            return -1;
        }
    }
}
int get_inicio_real_part(int byte_inicio_particion, bool es_logica){
    if(es_logica){
        int byte_inicio_real = byte_inicio_particion + sizeof(EBR);
    }else{
        return byte_inicio_particion;
    }
}
int get_tamanio_real_part(int tamanio_particion, bool es_logica){
    if(es_logica){
        int tamanio_real = tamanio_particion - sizeof(EBR);
    }else{
        return tamanio_particion;
    }
}

void reiniciar_montaje(){
    for (int i = 0; i < 9; ++i){
        for (int i_path = 0; i_path < 256; ++i_path){
            montaje[i].path[i_path] = '\0';
        }
        for (int j = 0; j < 32; ++j){
            for (int j_name = 0; j_name < 32; ++j_name){
                montaje[i].nombre_particion[j][j_name] = '\0';
            }
        }
    }
}

int convertir_a_numero_disco(char letra){
    if(letra == 'a'){
        return 0;
    }else if(letra == 'b'){
        return 1;
    }else if(letra == 'c'){
        return 2;
    }else if(letra == 'd'){
        return 3;
    }else if(letra == 'e'){
        return 4;
    }else if(letra == 'f'){
        return 5;
    }else if(letra == 'g'){
        return 6;
    }else if(letra == 'h'){
        return 7;
    }else if(letra == 'i'){
        return 8;
    }else if(letra == 'j'){
        return 9;
    }
}

char convertir_a_letra_disco(int numero){
    if(numero == 0){
        return 'a';
    }else if(numero == 1){
        return 'b';
    }else if(numero == 2){
        return 'c';
    }else if(numero == 3){
        return 'd';
    }else if(numero == 4){
        return 'e';
    }else if(numero == 5){
        return 'f';
    }else if(numero == 6){
        return 'g';
    }else if(numero == 7){
        return 'h';
    }else if(numero == 8){
        return 'i';
    }else if(numero == 9){
        return 'j';
    }
}

void agregarDisco(char *path){
    for (int i_encabezado = 0; i_encabezado < 9; ++i_encabezado){
        if(montaje[i_encabezado].path[0] == '\0'){
            strcpy(montaje[i_encabezado].path, path);
            return;
        }
    }
}

bool existeDisco(char *path){
    for (int i_encabezado = 0; i_encabezado < 9; ++i_encabezado){
        if(!strcmp(montaje[i_encabezado].path, path)){
            return true;
        }
    }
    return false;
}

bool existeParticion(char *name){
    for (int i_encabezado = 0; i_encabezado < 9; ++i_encabezado){
        if(montaje[i_encabezado].path[0] != '\0'){
            for (int i_name = 0; i_name < 32; ++i_name){
                if(!strcmp(montaje[i_encabezado].nombre_particion[i_name], name)){
                    return true;
                }
            }
        }
    }
    return false;
}

char get_letra_disco(char *path){
    for (int i_encabezado = 0; i_encabezado < 9; ++i_encabezado){
        if(!strcmp(montaje[i_encabezado].path, path)){
            return convertir_a_letra_disco(i_encabezado);
        }
    }
}

int get_numero_particion(char *name, char letra_disco){
    int num = convertir_a_numero_disco(letra_disco);
    //printf("Numero de montaje: %d\n", num);
    for (int i_name = 0; i_name < 32; ++i_name){
        if (!strcmp(montaje[num].nombre_particion[i_name], name)){
            return i_name + 1;
        }
    }
    return 0;
}

void agregarParticion(char *name, char letra_disco){
    int num = convertir_a_numero_disco(letra_disco);
    for (int i_name = 0; i_name < 32; ++i_name){
        if(montaje[num].nombre_particion[i_name][0] == '\0'){
            strcpy(montaje[num].nombre_particion[i_name], name);
            break;
        }
    }
    printf("Discos y particiones montados\n");
    for (int i_encabezado = 0; i_encabezado < 9; ++i_encabezado){
        if(montaje[i_encabezado].path[0] != '\0'){

            printf("Disco: '%c'\n", convertir_a_letra_disco(i_encabezado));
            for (int i_name = 0; i_name < 32; ++i_name){
                if(montaje[i_encabezado].nombre_particion[i_name][0] != '\0'){
                    printf("%d) %s\n", i_name+1, montaje[i_encabezado].nombre_particion[i_name]);
                }
            }
        }
    }
}

char * get_path(char *id){
    char letra_disco = id[2];
    return montaje[convertir_a_numero_disco(letra_disco)].path;
}

bool existeMontaje(char *id){
    char numero_string[2];
    numero_string[0] = id[3]; numero_string[1] = '\0';
    int numero_particion = atoi(numero_string);
    int num_disco = convertir_a_numero_disco(id[2]);

    if(montaje[num_disco].nombre_particion[numero_particion-1][0] != '\0'){
        return true;
    }else{
        return false;
    }
}

void desmontar(char *id){
    char numero_string[2];
    numero_string[0] = id[3]; numero_string[1] = '\0';
    int numero_particion = atoi(numero_string);
    int num_disco = convertir_a_numero_disco(id[2]);

    for (int i = 0; i < 32; ++i){
        montaje[num_disco].nombre_particion[numero_particion-1][i] = '\0';
    }

    bool estaVacio = true;
    for (int i = 0; i < 32; ++i){
        if(montaje[num_disco].nombre_particion[i][0] != '\0'){
            estaVacio = false;
            break;
        }
    }

    if(estaVacio){
        for (int i = 0; i < 256; ++i){
            montaje[num_disco].path[i] = '\0';
        }
    }
}

char * get_nombre(char *id){
    char numero_string[2];
    numero_string[0] = id[3]; numero_string[1] = '\0';
    int numero_particion = atoi(numero_string);
    int num_disco = convertir_a_numero_disco(id[2]);

    return montaje[num_disco].nombre_particion[numero_particion-1];
}
