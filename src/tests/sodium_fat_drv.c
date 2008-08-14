#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/mman.h>

#include <sodium_fat_drv.h>
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
uint16_t sodium_fat12_drv_get_constant( uint16_t constant ){
	switch( constant ){
		case( FAT_FREE_CLUSTER ): return FAT12_FREE_CLUSTER;
		case( FAT_RESERVED_CLUSTER ): return FAT12_RESERVED_CLUSTER;
		case( FAT_USED_CLUSTER ): return FAT12_USED_CLUSTER;
		case( FAT_BAD_CLUSTER ): return FAT12_BAD_CLUSTER;
		case( FAT_LAST_CLUSTER ): return FAT12_LAST_CLUSTER;
		default: return 0;
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
uint8_t* sodium_fat12_drv_get_cluster_addr( uint8_t *fat, uint16_t cluster ){
	return( (fat) + (cluster) * 3/2 );
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
uint16_t sodium_fat12_drv_get_next_cluster( uint8_t *fat, uint16_t cluster ){
	uint16_t aux = *((uint16_t*) 
			sodium_fat12_drv_get_cluster_addr( fat, cluster ));

	if( cluster % 2 ){
		return (aux & 0xfff0) >> 4;
	} else {
		return (aux & 0x0fff);
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
uint16_t sodium_fat12_drv_set_next_cluster( uint8_t *fat, 
					    uint16_t cluster, 
					    uint16_t next ){
	uint16_t *aux = (uint16_t*) 
			sodium_fat12_drv_get_cluster_addr( fat, cluster );

	if( cluster % 2 ){
		*aux = (next << 4) | (*aux & 0xf);
	} else {
		*aux = next | (*aux & 0xf000);
	}

	return *aux;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int device_init( struct device_handle_t *device,
		 const char *filename ){
	struct stat st;

	if( (device->fd = open( filename, O_RDWR )) < 0 ){
		perror( "open()" );
		return -1;
	}

	device->block_size = 512;

	fstat( device->fd, &st );
	device->total_blocks = st.st_size / device->block_size;

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int device_destroy( struct device_handle_t *device ){
	close( device->fd );

	return 0;
}

/* lee 'count' bloques a partir del bloque 'block' del dispositivo 
 * 'device', y los guarda en el área apuntada por 'buffer'.
 * devuelve la cantidad total de bloques leida, o -1 cuando hay algun error */
int device_read_blocks( struct device_handle_t *device,
		        uint32_t block,
		        uint32_t count,
		        uint8_t *buffer ){
	int ret = 0;

	/* posicionamos el file pointer al 'bloque' que queremos: */
	if( lseek( device->fd, block * device->block_size, SEEK_SET ) < 0 ){
		return -1;
	}

	/* leemos 'count' bloques: */
	while( count-- ){
		if( read( device->fd, buffer, device->block_size ) < 0 ){
			perror( "read()" );
			return -1;
		}
		buffer += device->block_size;
		++ret;
	}

	return ret;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int device_write_blocks( struct device_handle_t *device,
		         uint32_t block,
		         uint32_t count,
		         uint8_t *buffer ){
	int ret = 0;

	/* posicionamos el file pointer al 'bloque' que queremos: */
	if( lseek( device->fd, block * device->block_size, SEEK_SET ) < 0 ){
		return -1;
	}

	/* escribimos 'count' bloques: */
	while( count-- ){
		if( write( device->fd, buffer, device->block_size ) < 0 ){
			perror( "write()" );
			return -1;
		}
		buffer += device->block_size;
		++ret;
	}

	return ret;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
/* encapsula el mecanismo para obtener memoria */
void* kmalloc( uint32_t size ){
	return malloc( size );
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
/* encapsula el mecanismo para liberar memoria */
void kfree( void* area ){
	free( area );
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sodium_fat_drv_init( struct fs_fat_handle_t *handle ){
	/* inicialización primaria: */
	handle->header = NULL;
	handle->fat1 = NULL;
	handle->fat2 = NULL;

	handle->get_constant = NULL;
	handle->get_next_cluster = NULL;
	handle->set_next_cluster = NULL;
	
	/* leer el header: */
	handle->header = (struct fs_fat_header_t*) 
			 kmalloc( handle->device->block_size );

	if( !handle->header ){
		printf( "OOM: FAT header\n" );
		return -1;
	}

	device_read_blocks( handle->device,
			    0, /* bloque 0 */
			    1, /* leer un bloque */
			    (uint8_t*)handle->header );

	if( (handle->header->reserved_sectors + 
	     handle->header->sectors_per_fat) > handle->device->total_blocks ){
		printf( "Especificacion de FAT1 invalida\n" );
		return -1;
	}

	handle->fat1 = kmalloc( handle->header->sectors_per_fat * 
		       		handle->header->bytes_per_sector );

	if( !handle->fat1 ){
		printf( "OOM: FAT1\n" );
		return -1;
	}

	device_read_blocks( handle->device,
			    handle->header->reserved_sectors,
		       	    handle->header->sectors_per_fat,
		       	    handle->fat1 );

	handle->fat2 = kmalloc( handle->header->sectors_per_fat * 
		       		handle->header->bytes_per_sector );

	if( !handle->fat2 ){
		printf( "OOM: FAT2\n" );
		return -1;
	}
		
	device_read_blocks( handle->device,
			    handle->header->reserved_sectors + 
			    handle->header->sectors_per_fat,
		       	    handle->header->sectors_per_fat,
		       	    handle->fat2 );

	if( (handle->header->reserved_sectors + 
	     handle->header->sectors_per_fat * 2) > 
	    handle->device->total_blocks ){
		printf( "Especificacion de FAT2 invalida\n" );
		return -1;
	}
	
	if( handle->header->reserved_sectors +
	    handle->header->sectors_per_fat * handle->header->fats +
	    ((handle->header->root_entries * 
	      sizeof( struct fs_fat_dirent_t )) /
	     handle->device->block_size) > handle->device->total_blocks ){
		printf( "Especificacion de Root Dir invalida\n" );
		return -1;
	}
	
	if( !memcmp( handle->fat1, FAT16_HEADER, FAT16_HEADER_SIZE ) ){
		printf( "FAT: FAT16 (no soportada en este momento)\n" );
		return -1;
	} else if( !memcmp( handle->fat1, FAT12_HEADER, FAT12_HEADER_SIZE ) ){
		printf( "FAT: FAT12, asignando handlers\n" );
		handle->get_constant = sodium_fat12_drv_get_constant;
		handle->get_next_cluster = sodium_fat12_drv_get_next_cluster;
		handle->set_next_cluster = sodium_fat12_drv_set_next_cluster;
	} else if( !memcmp( handle->fat1, FAT12B_HEADER, FAT12_HEADER_SIZE ) ){
		printf( "FAT: FAT12, asignando handlers\n" );
		handle->get_constant = sodium_fat12_drv_get_constant;
		handle->get_next_cluster = sodium_fat12_drv_get_next_cluster;
		handle->set_next_cluster = sodium_fat12_drv_set_next_cluster;
	} else {
		printf( "Tipo FAT desconocida\n" );
		return -1;
	}

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sodium_fat_drv_sync( struct fs_fat_handle_t *handle ){
	/* copiamos la FAT1: */
	device_write_blocks( handle->device,
			     handle->header->reserved_sectors,
		       	     handle->header->sectors_per_fat,
		       	     handle->fat1 );

	/* copiamos la FAT1 en el lugar de la 2: */
	device_write_blocks( handle->device,
			     handle->header->reserved_sectors + 
			     handle->header->sectors_per_fat,
		       	     handle->header->sectors_per_fat,
		       	     handle->fat1 );

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sodium_fat_drv_destroy( struct fs_fat_handle_t *handle ){
	if( sodium_fat_drv_sync( handle ) )
		return -1;

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sodium_fat_drv_follow_chain( struct fs_fat_handle_t *handle, 
				 uint16_t cluster ){
	while( cluster < handle->get_constant( FAT_LAST_CLUSTER ) ) {
		cluster = handle->get_next_cluster( handle->fat1, cluster ); 
	}

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
/* la cuenta del numero de bloque adonde arranca el área de datos habría que
 * hacerla una sola vez al inicializar la estructura de FAT... */
uint16_t sodium_fat_drv_get_device_block( struct fs_fat_header_t *header, 
				          uint16_t cluster ){
	return header->reserved_sectors +
	       (header->fats * 
		header->sectors_per_fat) +
	       (header->root_entries * 
		sizeof( struct fs_fat_dirent_t )) / header->bytes_per_sector +
	       cluster - 2 * header->sectors_per_cluster;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
uint16_t sodium_fat_drv_get_root_dir_sector( struct fs_fat_header_t *header ){
	return header->reserved_sectors +
	       (header->fats * 
		header->sectors_per_fat);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
uint16_t sodium_fat_drv_build_chain( struct fs_fat_handle_t *handle, 
				     uint16_t clusters ){
	uint16_t prev_cluster = 3,
		 next_cluster,
		 initial_cluster = 3;

	/* buscamos el primer cluster disponible XXX buscar borrados tambien */
	while( handle->get_next_cluster( handle->fat1, initial_cluster ) )
		initial_cluster++;

	prev_cluster = initial_cluster;
	next_cluster = prev_cluster + 1;

	printf( "construccion_cadena(): cluster inicial: %u\n", 
		initial_cluster );

	while( clusters-- ){
		while( handle->get_next_cluster( handle->fat1, next_cluster ) )
			next_cluster++;

		handle->set_next_cluster( handle->fat1, 
					  prev_cluster, 
					  next_cluster );

		printf( "%u ", next_cluster );

		prev_cluster = next_cluster;
		++next_cluster;
	}

	printf( "\nconstruccion_cadena(): terminada\n" );

	handle->set_next_cluster( handle->fat1, 
				  prev_cluster, 
				  handle->get_constant( FAT_LAST_CLUSTER ) );
	
	return initial_cluster;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sodium_fat_drv_show_dirent( struct fs_fat_dirent_t *dirent ){
	printf( "%8.*s.%3.*s cluster [%4u] %10u bytes "
		"fecha: %4u/%02u/%02u %02u:%02u:%02u %c%c%c%c%c%c \n",
		sizeof( dirent->filename ), dirent->filename, 
		sizeof( dirent->extension ), dirent->extension,
	        dirent->starting_cluster,
	        dirent->filesize,
	     	FAT_DIRENT_DATE_YEAR( dirent->create_date ),
	     	FAT_DIRENT_DATE_MONTH( dirent->create_date ),
	     	FAT_DIRENT_DATE_DAY( dirent->create_date ),
	     	FAT_DIRENT_TIME_HOUR( dirent->create_time ),
	     	FAT_DIRENT_TIME_MIN( dirent->create_time ),
	     	FAT_DIRENT_TIME_SEC( dirent->create_time ),
	        dirent->attributes & FAT_DIRENT_ATTR_RDONLY  ? 'R' : '-',
	        dirent->attributes & FAT_DIRENT_ATTR_HIDDEN  ? 'H' : '-',
	        dirent->attributes & FAT_DIRENT_ATTR_SYSTEM  ? 'S' : '-',
	        dirent->attributes & FAT_DIRENT_ATTR_VOL_LBL ? 'V' : '-',
	        dirent->attributes & FAT_DIRENT_ATTR_SUBDIR  ? 'D' : '-',
	        dirent->attributes & FAT_DIRENT_ATTR_ARCHIVE ? 'A' : '-' );

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sodium_fat_drv_show_root_entries( struct fs_fat_handle_t *handle ){
	unsigned char sector[ handle->device->block_size ];
	int block, entry;
	struct fs_fat_dirent_t *dirent;

	for( block = 0; 
	     block < (handle->header->root_entries * 
		      sizeof( struct fs_fat_dirent_t)) / 
	     	     handle->device->block_size;
	     block++ ){
		device_read_blocks( 
				handle->device, 
				sodium_fat_drv_get_root_dir_sector( 
					handle->header ) + block,
				1,
				sector );
		for( entry = 0,
		     dirent = (struct fs_fat_dirent_t*) sector;
		     entry < (handle->device->block_size /
			      sizeof( struct fs_fat_dirent_t));
		     entry++,
		     dirent++ ){
			if( dirent->filename[0] &&
			    dirent->filename[0] != 0x05 &&
			    dirent->filename[0] != 0x2e &&
			    dirent->filename[0] != 0xe5 &&
			    dirent->starting_cluster ){
				sodium_fat_drv_show_dirent( dirent );
			}
		}
	}

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
uint8_t *str_to_upper( uint8_t *str, size_t size ){
	uint8_t *ptr = str;

	while( size-- ){
		*ptr = toupper( *ptr );
		++ptr;
	}

	return str;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sodium_fat_drv_add_dirent( struct fs_fat_handle_t *handle,
		       	       const char *filename,
		       	       uint8_t attributes,
			       uint16_t initial_cluster,
			       uint16_t filesize ){
	unsigned char sector[ handle->device->block_size ];
	int block, entry;
	struct fs_fat_dirent_t *dirent;
	const char *extension = NULL;

	/* buscamos entrada vacia (tambien deberiamos buscar borradas) */
	for( block = 0; 
	     block < (handle->header->root_entries * 
		      sizeof( struct fs_fat_dirent_t)) / 
	     	     handle->device->block_size;
	     block++ ){
		device_read_blocks( 
				handle->device, 
				sodium_fat_drv_get_root_dir_sector( 
					handle->header ) + block,
				1,
				sector );
		for( entry = 0,
		     dirent = (struct fs_fat_dirent_t*) sector;
		     entry < (handle->device->block_size /
			      sizeof( struct fs_fat_dirent_t ));
		     entry++,
		     dirent++ ){
			if( !dirent->filename[0] ){
				goto found;
			}
		}
	}

found:
	/* XXX q pasa si no hay entrada disponible */

	/* buscamos el punto que separa el nombre de la extension */
	if( !strchr( filename, '.' ) ){
		printf( "%s: el archivo no contiene '.'\n", filename );
		return -1;
	}

	if( strchr( filename, '.' ) != strrchr( filename, '.' ) ){
		printf( "%s: el archivo contiene mas de un '.'\n", filename );
		return -1;
	}

	extension = strchr( filename, '.' ) + 1;

	memset( dirent->filename, 0x20, sizeof( dirent->filename ) );
	memset( dirent->extension, 0x20, sizeof( dirent->extension ) );

	memcpy( dirent->filename, filename, extension - 1 - filename );
	memcpy( dirent->extension, extension, strlen( extension ) );

	str_to_upper( dirent->filename, strlen( filename ) );
	str_to_upper( dirent->extension, strlen( extension ) );

	dirent->attributes = attributes;
	dirent->starting_cluster = initial_cluster;
	dirent->filesize = filesize;
	
	/* commit the changes on the device: */
	device_write_blocks( handle->device,
			     sodium_fat_drv_get_root_dir_sector( 
			     	handle->header ) + block,
			     1,
			     sector );

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/	
int sodium_fat_drv_write_data( struct fs_fat_handle_t *handle,
			       uint8_t* data,
			       uint16_t cluster ){
	struct fs_fat_header_t *header = handle->header;
	uint8_t *fat = handle->fat1;

	printf( "escribiendo clusters: " );

	while( cluster < handle->get_constant( FAT_LAST_CLUSTER ) ) {
		printf( "%u ", cluster );
		device_write_blocks( handle->device,
				     sodium_fat_drv_get_device_block( 
					     handle->header,
					     cluster ),
				     header->sectors_per_cluster,
				     data );
		data += header->sectors_per_cluster
			* header->bytes_per_sector;
		cluster = handle->get_next_cluster( fat, cluster ); 
	}

	printf( "\nescritura completa\n" );

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sodium_fat_drv_show_header( struct fs_fat_handle_t *handle ){
	printf( "Dump header FAT (%p):\n"
		" * oem_id: %.*s\n"
	     	" * bytes_per_sector: %u\n"
		" * sectors_per_cluster: %u\n"
		" * reserved_sectors: %u\n"
		" * fats: %u\n"
		" * root_entries: %u\n"
		" * small_sectors: %u\n"
		" * media_descriptor: 0x%x\n"
		" * sectors_per_fat: %u\n"
		" * sectors_per_track: %u\n"
		" * heads: %u\n"
		" * hidden_sectors: %u\n"
		" * large_sectors: %u\n"
		" * physical_drive_number: 0x%x\n"
		" * current_head: %u\n"
		" * signature: 0x%x\n"
		" * volume_id: 0x%x\n"
	     	" * volume_name: %.*s\n"
	     	" * fs_type: %.*s\n",
		handle,
		sizeof( handle->header->oem_id ),
			handle->header->oem_id,
		handle->header->bytes_per_sector,
		handle->header->sectors_per_cluster,
		handle->header->reserved_sectors,
		handle->header->fats,
		handle->header->root_entries,
		handle->header->small_sectors,
		handle->header->media_descriptor,
		handle->header->sectors_per_fat,
		handle->header->sectors_per_track,
		handle->header->heads,
		handle->header->hidden_sectors,
		handle->header->large_sectors,
		handle->header->physical_drive_number,
		handle->header->current_head,
		handle->header->signature,
		handle->header->volume_id,
		sizeof( handle->header->volume_label ), 
			handle->header->volume_label,
		sizeof( handle->header->fs_type ), 
			handle->header->fs_type );

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int mmap_start( struct mmap_handle_t *handle, const char *drv ){
	struct stat st;

	if( (handle->fd = open( drv, O_RDWR )) < 0 ){
		perror( "open()" );
		return -1;
	}

	fstat( handle->fd, &st );

	handle->length = st.st_size;

	if( MAP_FAILED == (handle->mmap_area =
		mmap( NULL, handle->length, PROT_READ | PROT_WRITE, 
		      MAP_SHARED, handle->fd, 0 )) ){
		perror( "mmap()" );
		return -1;
	}

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int mmap_end( struct mmap_handle_t *handle ){
	munmap( handle->mmap_area, handle->length );

	close( handle->fd );

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int print_hexa( char *tag, const uint8_t* bin, size_t size ){
        const uint8_t *ptr = bin,
	                    *end_ptr = bin + size;

	printf( "%s: \n", tag );

	while( ptr < end_ptr ){
		printf( "%02hhx ", *ptr++ );
	}

	putchar( '\n' );

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int copy_external_file( struct fs_fat_handle_t *fat_handle,
			const char *filename,
			const char *dest_filename ){
	struct mmap_handle_t file_handle;
	uint16_t   total_sectors,
		   total_clusters,
		   initial_cluster;

	if( mmap_start( &file_handle, filename ) )
		return -1;

	total_sectors = file_handle.length / 
			fat_handle->header->bytes_per_sector,
	total_clusters = total_sectors / 
			 fat_handle->header->sectors_per_cluster,
	initial_cluster = sodium_fat_drv_build_chain( fat_handle, 
						      total_clusters );

	sodium_fat_drv_add_dirent( fat_handle,
		       		   strrchr( dest_filename, '/' ) 
				   	? strrchr( dest_filename, '/' ) + 1
					: dest_filename,
		       		   FAT_DIRENT_ATTR_ARCHIVE,
				   initial_cluster,
				   file_handle.length );

	sodium_fat_drv_write_data( fat_handle,
				   (uint8_t*)file_handle.mmap_area,
				   initial_cluster );

	mmap_end( &file_handle );

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int main( int argc, char *argv[] ){
	struct device_handle_t device;
	struct fs_fat_handle_t fat_handle;

	const char *img_file = NULL,
	      	   *filename = NULL,
	      	   *dest_filename = NULL;

	if( argc < 2 ){
		printf( "imagen FAT omitida \n" );
		return -1;
	}

	img_file = argv[ 1 ];

	if( argc > 2 )
		filename = argv[ 2 ];

	if( argc > 3 )
		dest_filename = argv[ 3 ];
	else
		dest_filename = filename;

	if( device_init( &device, img_file ) )
		return -1;

	fat_handle.device = &device;

	if( sodium_fat_drv_init( &fat_handle ) )
		return -1;

	if( sodium_fat_drv_show_header( &fat_handle ) )
		return -1;
  
	if( filename )
		copy_external_file( &fat_handle, filename, dest_filename );

	if( sodium_fat_drv_show_root_entries( &fat_handle ) )
		return -1;

#if 0
	print_hexa( "fat1", 
		    fat_handle.fat1, 
		    fat_handle.header->sectors_per_fat 
		    * fat_handle.header->bytes_per_sector );
#endif
#if 0
	print_hexa( "data_region",
		    fat_handle.data_region,
		    512 );
#endif

	if( sodium_fat_drv_destroy( &fat_handle ) )
		return -1;

	if( device_destroy( &device ) )
		return -1;

	return 0;
}

