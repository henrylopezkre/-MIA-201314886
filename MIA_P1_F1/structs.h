#ifndef STRUCTS_H
#define STRUCTS_H

struct struct_partition{
    int status_part;
    char type[5];
    char fit[3];
    int start_part;
    int size_part;
    char name[32];
};
struct struct_mbr{
    int tamanio_mbr;
    int restante_mbr;
    char fecha[100];
    int signature_mbr;
    int cant_particion_p;
    int cant_particion_e;
    struct struct_partition partition[4];
};
struct struct_ebr{
    char status_ebr;
    char fit[3];
    int start_ebr;
    int size_ebr;
    int next_ebr;
    char name[16];
};
struct struct_mount_disco{
    char path[100];
    int estado_mount;
    int letra_ascii;
};
struct struct_mount_particion{
    char name[50];
    int estado;
    int letra_ascii;
    int numero;
    char id[5];
};

#endif // STRUCTS_H
