#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include "structs.h"
typedef struct struct_mbr structMbr;
typedef struct struct_partition structPartition;
typedef struct struct_ebr structEbr;
typedef struct struct_mount_disco structMountDisco;
typedef struct struct_mount_particion structMountParticion;
int ends_with(char *str, char *suffix);
char* substr(char *string, int begin, int lenght);
void createDirs(char *path);
void crearDisco(char *size_i,char *unit,char *path);
void eliminarDisco(char *path);
int existeParticion(char*path_i,char*name_i);
int mountExisteDisco(char *path_i);
int mountExisteParticion(int letra_ascii);
int mountExisteParticion(int letra_ascii);
void mount(char *path_i, char *name_i);
void unmount(char *id_i);
char *getSentenciaComando(char *str_i);
char *getValorComando(char *str_i);
structMountDisco LIST_MOUNT_DISCO[27];
structMountParticion LIST_MOUNT_PARTICION[50];
int ASCII_LETRA;
int NUMEROLOGICAS=0;
int VECTOR_RANDOM[100];
struct stat st = {0};
int main(void){
    printf("Ingrese un nuevo comando.\n");
    comando();
    return 0;
}
void comando(char str[256]){
    int j=0;
    while (str[j]){
      str[j]= (tolower(str[j]));
      j++;
    }
    char *token;
    char *cmd[10];
    token = strtok(str, " ");
    int i=0;
    while( token != NULL ){
        cmd[i]=token;
        token = strtok(NULL, " ");
        i++;
    }
    if(strcmp(cmd[0],"mkdisk")==0){
        char *size="null",*unit="m",*path="null";
        int j;
        for(j=1;j<i;j++){
            if(strcmp(getSentenciaComando(cmd[j]),"-size")==0){
                size=getValorComando(cmd[j]);
            }else if(strcmp(getSentenciaComando(cmd[j]),"-unit")==0){
                unit=getValorComando(cmd[j]);
            }else if(strcmp(getSentenciaComando(cmd[j]),"-path")==0){
                path=getValorComando(cmd[j]);
            }else if(cmd[j][0]=='#'){
                break;
            }
        }
        if(strcmp(size,"null")==0||strcmp(path,"null")==0){
            printf("No se pudo crear el disco intente de nuevo\n");
        }else{
          //
        }
    }
  }
}
char *getSentenciaComando(char *str_i){
    char *str =str_i;
    char *token,*cmd[2];
    token= strtok(str,":");
    int j=0;
    while(token!=NULL){
        cmd[j]=token;
        token = strtok(NULL,":");
        j++;
    }
    return cmd[0];
}
void script(char *path){
    FILE *file;
    char *c, cad[256];
    file=fopen(path, "rb");
    if(file==NULL){
        printf("El archivo no existe.");
    }else{
        while((c=fgets(cad,256,file)))
            comandoExec(cad);
    }
    fclose(file);
}
char *getValorComando(char *str_i){
    char *str =str_i;
    char *token="null",*cmd[2] = {"null","null"};
    token= strtok(str,":");
    int j=0;
    while(token!=NULL){
        cmd[j]=token;
        token = strtok(NULL,":");
        j++;
    }
    return cmd[1];
}

int ends_with(char *str, char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}
void createDirs(char *path){
    char path_i[1024] = "";
    strcpy(path_i, path);
    if(strlen(path_i) != 0){
        char* dir = dirname(path_i);
            if (stat(dir, &st) == -1) {
                char dirs[80] = "mkdir -p ";
                strcat(dirs, path_i);
                system(dirs);
            }
    }
}

void crearDisco(char *size_i,char *unit,char *path){
    createDirs(path);
    FILE *file=fopen(path,"wb");
    char n='\0';
    int i,tamannoByte,size=atoi(size_i);
    if(strcmp(unit,"k")==0){
        tamannoByte=size*1024;
    }else if(strcmp(unit,"m")==0){
        tamannoByte=size*1048576;
    }
    for(i=0;i<tamannoByte;i++){
        fwrite(&n,sizeof(n),1,file);
    }
    fclose(file);
    crearMbr(path,tamannoByte);
    printf("Se creo un nuevo disco\n");
}
void eliminarDisco(char *path){
    char opcion;
    FILE * file =fopen(path,"r");
    if(file){
        fclose(file);
        printf("¿ Desea eliminar el  disco: %s ?\n",path);
        printf("Si[y], No[n].");
        scanf(" %c",&opcion);
        if((opcion=='y')||(opcion=='Y')){
            if(remove(path)==0){
                printf("Disco eliminado\n");
            }
        }else{
            printf("se cancelo la eliminacion\n");
        }


    }else{
        printf("El archivo no se encuentra\n");
    }
}

void crearMbr(char *path,int tamannoDisco){
    structMbr mbr;
    structPartition part;
    part.status=0;
    part.start=0;
    part.size=0;
    strcpy(part.type,"null");
    time_t tiempo = time(0);
    struct tm *tlocal = localtime(&tiempo);
    char output[128];
    strftime(output,128,"%d/%m/%y %H:%M:%S",tlocal);
    mbr.tamanno=tamannoDisco;
    mbr.restante=tamannoDisco-sizeof(structMbr);
    strcpy(mbr.fecha,output);
    mbr.signature=randomSignature();
    mbr.cant_particion_p=0;
    mbr.cant_particion_e=0;
    int i=0;
    for(i=0;i<4;i++){
        mbr.partition[i]=part;
    }
    FILE *file =fopen(path,"r+");
    fwrite(&mbr,sizeof(structMbr),1,file);
    fclose(file);
}

void crearParticion(char *size_i, char   *unit_i, char *path_i,char *type_i, char *fit_i, char *name_i){
    FILE *file = fopen(path_i,"r+");
    structMbr mbr;
    int unit=1,size=atoi(size_i);
    if(strcmp(unit_i,"k")==0){
        unit=1024;
    }else if(strcmp(unit_i,"m")==0){
        unit=1048576;
    }

    if(file){
            fread(&mbr,sizeof(structMbr),1,file);
            int continuar;
            if(strcmp(type_i,"p")==0){
                if(mbr.cant_particion_p<4){
                    continuar=1;
                }else{
                    continuar=0;
                    printf("No se puede crear mas particiones primarias\n");
                }
            }else if(strcmp(type_i,"e")==0){
                if(mbr.cant_particion_p<4){
                    if(mbr.cant_particion_e==1){
                        continuar=0;
                        printf("No se puede agregar otra particion extendida\n");
                    }else{
                        continuar=1;
                    }
                }else{
                    continuar=0;
                    printf("No se puede crear mas particiones\n");
                }
            }else if(strcmp(type_i,"l")==0){
                if(mbr.cant_particion_e==1){
                    continuar=1;
                }else{
                    continuar=0;
                    printf("No existe particion extendida para crear logicas\n");
                }
            }else{
                printf("Tipo de particion desconocido\n");
                continuar=0;
            }
            if(continuar==1){
                if(strcmp(type_i,"l")==0){
                    structEbr ebr;
                    int i,byteInicio=-1,maximo=-1;
                    for(i=0;i<4;i++){
                        if(mbr.partition[i].type[0]=='e'){
                            byteInicio=mbr.partition[i].start+1;
                            maximo=mbr.partition[i].start +mbr.partition[i].size;
                            break;
                        }
                    }
                    fseek(file,byteInicio,SEEK_SET);
                    if(NUMEROLOGICAS==0){
                        ebr.status=1;
                        strcpy(ebr.fit,fit_i);
                        ebr.start=byteInicio+sizeof(structEbr)+1;
                        ebr.size=size*unit;
                        ebr.next=-1;
                        strcpy(ebr.name,name_i);
                        if(maximo>(ebr.start+ebr.size)){
                            fwrite(&ebr,sizeof(ebr),1,file);
                            NUMEROLOGICAS++;
                            printf("Se agrego una nueva particion logica\n");
                        }else{
                            printf("No existe mas espacio en la particion extendida\n");
                        }

                    }else{
                        fread(&ebr,sizeof(structEbr),1,file);
                        int i=0;
                        while(i<50){
                            byteInicio=ebr.start+ebr.size+1;
                            if(ebr.next==-1){
                                ebr.next=byteInicio;
                                fseek(file,-sizeof(structEbr),SEEK_CUR);
                                fwrite(&ebr,sizeof(structEbr),1,file);
                                break;
                            }
                            fseek(file,byteInicio,SEEK_SET);
                            fread(&ebr,sizeof(structEbr),1,file);
                            i++;
                        }
                            fseek(file,byteInicio,SEEK_SET);
                            ebr.status=1;
                            strcpy(ebr.fit,fit_i);
                            ebr.start=byteInicio+sizeof(structEbr)+1;
                            ebr.size=size*unit;
                            ebr.next=-1;
                            strcpy(ebr.name,name_i);
                        if(maximo>(ebr.start+ebr.size)){
                            fwrite(&ebr,sizeof(structEbr),1,file);
                            NUMEROLOGICAS++;
                            printf("Se agrego una nueva particion logica\n");
                        }else{
                            printf("No existe mas espacio en la particion extendida\n");
                        }
                    }
                }else{
                    structPartition particion;
                    int i, byte_inicio=sizeof(structMbr);
                    for(i=0;i<4;i++){
                        if(mbr.partition[i].status==0&&mbr.partition[i].start==0){
                            if(((size*unit))>mbr.restante){
                                printf("No hay suficiente espacio para la particion\n");
                                break;
                            }else{
                                particion.status=1;
                                strcpy(particion.type,type_i);
                                strcpy(particion.fit,fit_i);
                                particion.start=byte_inicio+1;
                                particion.size=size*unit;
                                strcpy(particion.name,name_i);
                                mbr.partition[i]=particion;
                                mbr.restante=mbr.restante-size*unit;
                                if(strcmp(type_i,"p")==0){
                                    mbr.cant_particion_p++;
                                }else{
                                    mbr.cant_particion_e=1;
                                }
                                fseek(file,0,SEEK_SET);
                                fwrite(&mbr,sizeof(structMbr),1,file);
                                printf("Se agrego una nueva particion\n");
                                break;
                            }
                        }else{
                            if(mbr.partition[i].status==0&&(size*unit)<=mbr.partition[i].size){
                                printf("Cabe en la particion %d\n",i);
                                particion.status=1;
                                strcpy(particion.type,type_i);
                                strcpy(particion.fit,fit_i);
                                particion.start=mbr.partition[i].start;
                                particion.size=mbr.partition[i].size;
                                strcpy(particion.name,name_i);
                                mbr.partition[i]=particion;
                                mbr.restante=mbr.restante-particion.size;
                                if(strcmp(type_i,"p")==0){
                                    mbr.cant_particion_p++;
                                }else{
                                    mbr.cant_particion_e=1;
                                }
                                fseek(file,0,SEEK_SET);
                                fwrite(&mbr,sizeof(structMbr),1,file);
                                printf("Se agrego una nueva particion\n");
                                break;
                            }else if(mbr.partition[i].status==0&&(size*unit)>mbr.partition[i].size){
                                printf("No hay suficiente espacio para la particion\n");
                            }else{
                                byte_inicio=(mbr.partition[i].start+mbr.partition[i].size);
                            }
                        }
                    }
                }
            }
        fclose(file);
    }else{
        printf("El disco no se encontro <%s>",path_i);
    }
}
int mountExisteDisco(char *path_i){
    int i=0;
    while(i<27){
        if(strcmp(LIST_MOUNT_DISCO[i].path,path_i)==0){
            break;
        }
        i++;
    }
    return i;
}
void mount(char *path_i, char *name_i) {
    structMountDisco temp;
    if(existeParticion(path_i,name_i)==1){
        int verificarDisco=mountExisteDisco(path_i);
        if(verificarDisco<27){
            temp= LIST_MOUNT_DISCO[verificarDisco];
        }else{
            int i=0;
            while(i<27){
                if(LIST_MOUNT_DISCO[i].estado==0){
                    temp.letra_ascii=ASCII_LETRA;
                    temp.estado=1;
                    strcpy(temp.path,path_i);
                    LIST_MOUNT_DISCO[i]=temp;
                    ASCII_LETRA++;
                    break;
                }
                i++;
            }
        }
        structMountParticion temporal;
        int vereficarParticion=mountExisteParticion(temp.letra_ascii);
        int i=0;
        while(i<50){
            if(LIST_MOUNT_PARTICION[i].numero==0){
                strcpy(LIST_MOUNT_PARTICION[i].name,name_i);
                temporal.estado=1;
                temporal.letra_ascii=temp.letra_ascii;
                temporal.numero=vereficarParticion+1;
                    temporal.id[0]= 'v';
                    temporal.id[1]= 'd';
                    temporal.id[2]= temporal.letra_ascii;
                    temporal.id[3]= temporal.numero+48;
                    temporal.id[4]= '\0';
                LIST_MOUNT_PARTICION[i]=temporal;
                printf("Particion montada vd%c%c\n",temporal.id[2],temporal.id[3]);
                break;
            }
            i++;
        }
    }
}
void crearParticion(char *size_i, char   *unit_i, char *path_i,char *type_i, char *fit_i, char *name_i){
    FILE *file = fopen(path_i,"r+");
    structMbr mbr;
    int unit=1,size=atoi(size_i);
    if(strcmp(unit_i,"k")==0){
        unit=1024;
    }else if(strcmp(unit_i,"m")==0){
        unit=1048576;
    }

    if(file){
            fread(&mbr,sizeof(structMbr),1,file);
            int continuar;
            if(strcmp(type_i,"p")==0){
                if(mbr.cant_particion_p<4){
                    continuar=1;
                }else{
                    continuar=0;
                    printf("No se puede crear mas particiones primarias\n");
                }
            }else if(strcmp(type_i,"e")==0){
                if(mbr.cant_particion_p<4){
                    if(mbr.cant_particion_e==1){
                        continuar=0;
                        printf("No se puede agregar otra particion extendida\n");
                    }else{
                        continuar=1;
                    }
                }else{
                    continuar=0;
                    printf("No se puede crear mas particiones\n");
                }
            }else if(strcmp(type_i,"l")==0){
                if(mbr.cant_particion_e==1){
                    continuar=1;
                }else{
                    continuar=0;
                    printf("No existe particion extendida para crear logicas\n");
                }
            }else{
                printf("Tipo de particion desconocido\n");
                continuar=0;
            }
            if(continuar==1){
                if(strcmp(type_i,"l")==0){
                    structEbr ebr;
                    int i,byteInicio=-1,maximo=-1;
                    for(i=0;i<4;i++){
                        if(mbr.partition[i].type[0]=='e'){
                            byteInicio=mbr.partition[i].start+1;
                            maximo=mbr.partition[i].start +mbr.partition[i].size;
                            break;
                        }
                    }
                    fseek(file,byteInicio,SEEK_SET);
                    if(NUMEROLOGICAS==0){
                        ebr.status=1;
                        strcpy(ebr.fit,fit_i);
                        ebr.start=byteInicio+sizeof(structEbr)+1;
                        ebr.size=size*unit;
                        ebr.next=-1;
                        strcpy(ebr.name,name_i);
                        if(maximo>(ebr.start+ebr.size)){
                            fwrite(&ebr,sizeof(ebr),1,file);
                            NUMEROLOGICAS++;
                            printf("Se agrego una nueva particion logica\n");
                        }else{
                            printf("No existe mas espacio en la particion extendida\n");
                        }

                    }else{
                        fread(&ebr,sizeof(structEbr),1,file);
                        int i=0;
                        while(i<50){
                            byteInicio=ebr.start+ebr.size+1;
                            if(ebr.next==-1){
                                ebr.next=byteInicio;
                                fseek(file,-sizeof(structEbr),SEEK_CUR);
                                fwrite(&ebr,sizeof(structEbr),1,file);
                                break;
                            }
                            fseek(file,byteInicio,SEEK_SET);
                            fread(&ebr,sizeof(structEbr),1,file);
                            i++;
                        }
                            fseek(file,byteInicio,SEEK_SET);
                            ebr.status=1;
                            strcpy(ebr.fit,fit_i);
                            ebr.start=byteInicio+sizeof(structEbr)+1;
                            ebr.size=size*unit;
                            ebr.next=-1;
                            strcpy(ebr.name,name_i);
                        if(maximo>(ebr.start+ebr.size)){
                            fwrite(&ebr,sizeof(structEbr),1,file);
                            NUMEROLOGICAS++;
                            printf("Se agrego una nueva particion logica\n");
                        }else{
                            printf("No existe mas espacio en la particion extendida\n");
                        }
                    }
                }else{
                    structPartition particion;
                    int i, byte_inicio=sizeof(structMbr);
                    for(i=0;i<4;i++){
                        if(mbr.partition[i].status==0&&mbr.partition[i].start==0){
                            if(((size*unit))>mbr.restante){
                                printf("No hay suficiente espacio para la particion\n");
                                break;
                            }else{
                                particion.status=1;
                                strcpy(particion.type,type_i);
                                strcpy(particion.fit,fit_i);
                                particion.start=byte_inicio+1;
                                particion.size=size*unit;
                                strcpy(particion.name,name_i);
                                mbr.partition[i]=particion;
                                mbr.restante=mbr.restante-size*unit;
                                if(strcmp(type_i,"p")==0){
                                    mbr.cant_particion_p++;
                                }else{
                                    mbr.cant_particion_e=1;
                                }
                                fseek(file,0,SEEK_SET);
                                fwrite(&mbr,sizeof(structMbr),1,file);
                                printf("Se agrego una nueva particion\n");
                                break;
                            }
                        }else{
                            if(mbr.partition[i].status==0&&(size*unit)<=mbr.partition[i].size){
                                printf("Cabe en la particion %d\n",i);
                                particion.status=1;
                                strcpy(particion.type,type_i);
                                strcpy(particion.fit,fit_i);
                                particion.start=mbr.partition[i].start;
                                particion.size=mbr.partition[i].size;
                                strcpy(particion.name,name_i);
                                mbr.partition[i]=particion;
                                mbr.restante=mbr.restante-particion.size;
                                if(strcmp(type_i,"p")==0){
                                    mbr.cant_particion_p++;
                                }else{
                                    mbr.cant_particion_e=1;
                                }
                                fseek(file,0,SEEK_SET);
                                fwrite(&mbr,sizeof(structMbr),1,file);
                                printf("Se agrego una nueva particion\n");
                                break;
                            }else if(mbr.partition[i].status==0&&(size*unit)>mbr.partition[i].size){
                                printf("No hay suficiente espacio para la particion\n");
                            }else{
                                byte_inicio=(mbr.partition[i].start+mbr.partition[i].size);
                            }
                        }
                    }
                }
            }
        fclose(file);
    }else{
        printf("El disco no se encontro <%s>",path_i);
    }
}

void eliminarParticion(char *name_i,char *mode_delete, char*path_i){
    FILE *file = fopen(path_i,"r+");
    structMbr mbr;
    if(file){
        fread(&mbr,sizeof(structMbr),1,file);
        int i, existeParticion=1;
        if(strcmp(mode_delete,"fast")==0){
            for(i=0;i<4;i++){
                if(strcmp(mbr.partition[i].name,name_i)==0){
                    char opcion;
                    printf("¿ Desea eliminar la Particion: %s ?\n",name_i);
                    printf("Si[y], No[n].");
                    scanf(" %c",&opcion);
                    if((opcion=='y')||(opcion=='Y')){
                        if(strcmp(mbr.partition[i].type,"p")==0){
                            mbr.partition[i].status=0;
                            mbr.restante= mbr.restante + mbr.partition[i].size;
                            mbr.cant_particion_p--;
                        }else if(strcmp(mbr.partition[i].type,"e")==0){
                            mbr.partition[i].status=0;
                            mbr.restante= mbr.restante + mbr.partition[i].size;
                            mbr.cant_particion_e=0;
                        }
                        printf("particion %s fue eliminada\n",name_i);
                        break;
                    }else{
                        printf("se cancelo la eliminacion de particion\n");
                        break;
                    }
                }else{
                    if(existeParticion==4){
                        printf("La particion no se encontro o no existe\n");
                    }
                    existeParticion++;
                }
            }
            fseek(file,0,SEEK_SET);
            fwrite(&mbr,sizeof(structMbr),1,file);
        }else if(strcmp(mode_delete,"full")==0){
            for(i=0;i<4;i++){
                if(strcmp(mbr.partition[i].name,name_i)==0){
                    char opcion;
                    printf("¿ Desea eliminar la Particion: %s ?\n",name_i);
                    printf("Si[y], No[n].");
                    scanf(" %c",&opcion);
                    if((opcion=='y')||(opcion=='Y')){
                        int byteInicio=mbr.partition[i].start;
                        int byteFin=byteInicio+mbr.partition[i].size;
                        if(strcmp(mbr.partition[i].type,"p")==0){
                            mbr.partition[i].status=0;
                            mbr.restante= mbr.restante + mbr.partition[i].size;
                            mbr.cant_particion_p--;
                        }else if(strcmp(mbr.partition[i].type,"e")==0){
                            mbr.partition[i].status=0;
                            mbr.restante= mbr.restante + mbr.partition[i].size;
                            mbr.cant_particion_e=0;
                        }
                        fseek(file,byteInicio,SEEK_SET);
                        char n='\0';
                        int i;
                        for(i=byteInicio;i<byteFin;i++){
                            fwrite(&n,sizeof(n),1,file);
                        }
                        printf("particion %s fue eliminada\n",name_i);
                        break;
                    }else{
                        printf("se cancelo la eliminacion de particion\n");
                        break;
                    }
                }else{
                    if(existeParticion==4){
                        printf("La particion no se encontro o no existe\n");
                    }
                    existeParticion++;
                }
            }
            fseek(file,0,SEEK_SET);
            fwrite(&mbr,sizeof(structMbr),1,file);
        }
        fclose(file);
    }else{
        printf("No se pudo eliminar particion: path desconocido...\n");
    }
}
