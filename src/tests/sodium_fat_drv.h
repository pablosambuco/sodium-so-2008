#ifndef __SODIUM_FAT_DRV_H__
#define    __SODIUM_FAT_DRV_H__

typedef unsigned char 	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned int 	uint32_t;

/* para poder manejar dinamicamente FAT12 y 16, aislamos el codigo dependiente 
 * de cada uno en funciones separadas, y al handle le cargamos punteros a esas
 * funciones (late-binding comun y corriente) */
typedef uint16_t (*fat_get_constant_hn)( uint16_t constant );
typedef uint16_t (*fat_get_next_cluster_hn)( uint8_t *fat, 
					     uint16_t cluster );
typedef uint16_t (*fat_set_next_cluster_hn)( uint8_t *fat, 
					     uint16_t cluster, 
					     uint16_t next );

/* describe un file system FAT (cualquier version) */
struct fs_fat_header_t {
	 uint8_t ignored[ 3 ];
	 uint8_t oem_id[ 8 ];
	uint16_t bytes_per_sector;
	 uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	 uint8_t fats;
	uint16_t root_entries;
	uint16_t small_sectors;
	 uint8_t media_descriptor;
	uint16_t sectors_per_fat;
	uint16_t sectors_per_track;
	uint16_t heads;
	uint32_t hidden_sectors;
	uint32_t large_sectors;
	 uint8_t physical_drive_number;
	 uint8_t current_head;
	 uint8_t signature;
	uint32_t volume_id;
	 uint8_t volume_label[ 11 ];
	 uint8_t fs_type[ 8 ];
} __attribute__ ((packed));

/* entrada en un directorio FAT */
struct fs_fat_dirent_t {
	 uint8_t filename[8];
	 uint8_t extension[3];
	 uint8_t attributes;
	 uint8_t reserved;
	 uint8_t create_time_fine;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t access_date;
	uint16_t ea_index;
	uint16_t modified_time;
	uint16_t modified_date;
	uint16_t starting_cluster;
	uint32_t filesize;
} __attribute__ ((packed));

/* helper para determinar que campo del header de FAT tomar para saber la
 * cantidad total de sectores que ocupa el FS (cuando la cantidad de sectores
 * supera lo que puede almacenarse en small_sectors, este va en cero y la 
 * cantidad se pone en el segundo. fucking Microsoft y sus hacks) */
#define FAT_TOTAL_SECTORS( header ) \
		(((header)->small_sectors) \
		 	? (header->small_sectors) \
		 	: (header->large_sectors))

/* atributos de las entradas de directorio (archivos y directorios) */
#define FAT_DIRENT_ATTR_RDONLY  0x01 /* Read Only */
#define FAT_DIRENT_ATTR_HIDDEN  0x02 /* Hidden */
#define FAT_DIRENT_ATTR_SYSTEM  0x04 /* System */
#define FAT_DIRENT_ATTR_VOL_LBL 0x08 /* Volume Label */
#define FAT_DIRENT_ATTR_SUBDIR  0x10 /* Subdirectory */
#define FAT_DIRENT_ATTR_ARCHIVE 0x20 /* Archive */
#define FAT_DIRENT_ATTR_DEVICE  0x40 /* Device (internal use only, never found 
					on disk) */
#define FAT_DIRENT_ATTR_UNUSED  0x80 /* Unused */

/* macros para extraer los subcomponetes de la fecha (hora y dia) de cada
 * entrada */
#define FAT_DIRENT_DATE_DAY( date ) \
	 (((date) & 0x001f) >> 0) 	  /*  4-0: 0000 0000 0001 1111 */
#define FAT_DIRENT_DATE_MONTH( date ) \
	 (((date) & 0x01e0) >> 5) 	  /*  8-5: 0000 0001 1110 0000 */
#define FAT_DIRENT_DATE_YEAR( date ) \
	((((date) & 0xfe00) >> 9) + 1980) /* 15-9: 1111 1110 0000 0000 */

#define FAT_DIRENT_TIME_SEC( time ) \
	((((time) & 0x001f) >> 0) * 2)	 /*   4-0: 0000 0000 0001 1111 */
#define FAT_DIRENT_TIME_MIN( time ) \
	 (((time) & 0x07e0) >> 5) 	 /*  10-5: 0000 0111 1110 0000 */
#define FAT_DIRENT_TIME_HOUR( time ) \
	 (((time) & 0xf800) >> 11)	 /* 15-11: 1111 1000 0000 0000 */

/* magic number que tienen las FAT (para cada version) */
#define FAT12_HEADER "\xf8\xff\xff"
#define FAT12B_HEADER "\xf0\xff\xff"
#define FAT16_HEADER "\xf8\xff\xff\xff"

/** otras constantes dependientes de la version de FAT **/
/* size del header */
#define FAT12_HEADER_SIZE 3
#define FAT16_HEADER_SIZE 4

/* identificacion de clusters */
#define FAT12_FREE_CLUSTER 	0x000  /* Free Cluster */
#define FAT12_RESERVED_CLUSTER 	0x001  /* Reserved Cluster */
#define FAT12_USED_CLUSTER 	0x002  /* Used cluster; value points to next 
					  cluster */
#define FAT12_BAD_CLUSTER 	0xFF7  /* Bad cluster */
#define FAT12_LAST_CLUSTER 	0xFF8  /* Last cluster in file */

#define FAT16_FREE_CLUSTER 	0x0000 /* Free Cluster */
#define FAT16_RESERVED_CLUSTER 	0x0001 /* Reserved Cluster */
#define FAT16_USED_CLUSTER 	0x0002 /* Used cluster; value points to next 
					  cluster */
#define FAT16_BAD_CLUSTER 	0xFFF7 /* Bad cluster */
#define FAT16_LAST_CLUSTER 	0xFFF8 /* Last cluster in file */

/* defines para identificar cada tipo de cluster */
#define FAT_FREE_CLUSTER 	1
#define FAT_RESERVED_CLUSTER 	2
#define FAT_USED_CLUSTER 	3
#define FAT_BAD_CLUSTER 	4
#define FAT_LAST_CLUSTER 	5

/* facilita el acceso a un archivo, debe moverse a un .h separado */
struct mmap_handle_t {
	int fd;
	void *mmap_area;
	size_t length;
};

/* encapsula el acceso al dispositivo que contiene el FS (dentro de SODIUM
 * se mapeará al cache de bloques) */
struct device_handle_t {
	int fd;
	uint32_t block_size,
		 total_blocks;
};

/* handle para las operaciones sobre FAT */
struct fs_fat_handle_t {
	struct device_handle_t *device;
	struct fs_fat_header_t *header;
	uint8_t *fat1,
		*fat2;

	/* funciones para adaptarse dinamicamente a FAT12/16: */
	fat_get_constant_hn get_constant;
	fat_get_next_cluster_hn get_next_cluster;
	fat_set_next_cluster_hn set_next_cluster;
};

#endif
