
#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct S_Instruccion{
    char comando[256];

    bool tiene_path;
    char path[256];

    bool tiene_size;
    unsigned int size;

    bool tiene_unit;
    char unit;

    bool tiene_name;
    char name[256];

    bool tiene_type;
    bool tiene_type_2;
    char type;
    char type_2[8];

    bool tiene_fit;
    char fit[4];

    bool tiene_delete;
    char delete[8];

    bool tiene_add;
    signed int add;

    bool tiene_id;
    char id[16];

    bool tiene_ruta;
    char ruta[256];

}S_Instruccion;

typedef struct Partition{
    char part_status;
    char part_type;
    char part_fit;
    unsigned int part_start;
    unsigned int part_size;
    char part_name[16];
}Partition;

typedef struct MBR{
    unsigned int mbr_tamanio;
    time_t mbr_fecha_creacion;
    unsigned int mbr_disk_signature;
    Partition mbr_partition_1;
    Partition mbr_partition_2;
    Partition mbr_partition_3;
    Partition mbr_partition_4;
}MBR;

typedef struct EBR{
    char part_status;
    char part_fit;
    unsigned int part_start;
    unsigned int part_size;
    unsigned int part_next;
    char part_name[16];
}EBR;

typedef struct EncabezadoMontaje{
    char path[256];
    char nombre_particion[32][32];
}EncabezadoMontaje;


#endif // STRUCTS_H
