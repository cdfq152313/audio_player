#include <string.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <unistd.h>
#include "fio.h"
#include "filesystem.h"
#include "romfs.h"
#include "osdebug.h"
#include "hash-djb2.h"

#define TRUE 1
#define FALSE 0

#define index_block_size 64
#define content_block_size 1024
#define filename_size 51

#define offset_hashvalue 0
#define offset_is_directory 4
#define offset_filename 5
#define offset_datasize 56 
#define offset_data_address 60

struct romfs_fds_t {
    const uint8_t * file;
    uint32_t size;
    uint32_t cursor;
};

static struct romfs_fds_t romfs_fds[MAX_FDS];

static uint32_t get_unaligned(const uint8_t * d) {
    return ((uint32_t) d[0]) | ((uint32_t) (d[1] << 8)) | ((uint32_t) (d[2] << 16)) | ((uint32_t) (d[3] << 24));
}

static ssize_t romfs_read(void * opaque, void * buf, size_t count) {
    struct romfs_fds_t * f = (struct romfs_fds_t *) opaque;
    
    if ((f->cursor + count) > f->size)
        count = f->size - f->cursor;

    memcpy(buf, f->file + f->cursor, count);
    f->cursor += count;

    return count;
}

static off_t romfs_seek(void * opaque, off_t offset, int whence) {
    struct romfs_fds_t * f = (struct romfs_fds_t *) opaque;
    const uint8_t * size_p = f->file - 4;
    uint32_t size = get_unaligned(size_p);
    uint32_t origin;
    
    switch (whence) {
    case SEEK_SET:
        origin = 0;
        break;
    case SEEK_CUR:
        origin = f->cursor;
        break;
    case SEEK_END:
        origin = size;
        break;
    default:
        return -1;
    }

    offset = origin + offset;

    if (offset < 0)
        return -1;
    if (offset > size)
        offset = size;

    f->cursor = offset;

    return offset;
}

const uint8_t * block2address(const uint8_t * opaque,uint32_t block,uint8_t is_directory){
    const uint8_t * meta = opaque;
    uint32_t index_count;
    index_count = get_unaligned(meta);
    meta += 8;

    if(is_directory){
        return meta + index_block_size * block;
    }
    else{
        meta += index_block_size * index_count;
        return meta + content_block_size * block;
    }

}

const uint8_t * romfs_get_index_by_hash(const uint8_t * romfs, uint32_t h, uint32_t _max) {
    const uint8_t * meta;
    uint32_t max =  _max;

    for (meta = romfs; max-- ; meta += index_block_size) {
        if (get_unaligned(meta) == h) {
            return meta ;
        }
    }

    return NULL;
}

const uint8_t * romfs_get_address_by_index(const uint8_t * opaque, const uint8_t * meta, uint8_t * is_directory, uint32_t *  datasize){
    uint32_t block;
    //get directory address
    *is_directory = *(meta +  offset_is_directory);
    *datasize = get_unaligned(meta + offset_datasize);

    block = get_unaligned(meta + offset_data_address);
    return block2address(opaque, block, *is_directory);
}

const uint8_t * romfs_get_address_by_path(const uint8_t * opaque, const char * path, uint8_t * is_directory, uint32_t *  datasize){
    const uint8_t * meta = (const uint8_t *) opaque;
    const char * slash;
    const char * init_path;
    uint32_t init_hash = 0;
    uint32_t hash = 0;

    meta += 4;
    * datasize = get_unaligned(meta);
    * is_directory = TRUE ;
    meta += 4;

    //root
    while(path[0] == '/')
        path++;
    if(path[0] == '\0')
        return meta;

    init_path = path;

    while(1){
        //previous path isn't directory
        if( !(*is_directory) )
            return 0;

        //get current hash value
        slash = strchr(path, '/');
        if(slash){
            hash = hash_djb2( (const uint8_t *)path, init_hash, slash - path);
            init_hash = hash_djb2( (const uint8_t *)init_path, 0, slash - init_path + 1);
        }
        else{
            hash = hash_djb2( (const uint8_t *)path, init_hash, -1);
        }
 
        //get address
        meta = romfs_get_index_by_hash(meta, hash, *datasize);
        if(!meta)
            return 0;
        meta = romfs_get_address_by_index(opaque, meta, is_directory, datasize);

        //go to next path
        if(!slash)
            break;
        path = slash + 1;
        while(path[0]== '/')
            path++;
        if(path[0] == '\0')
            break;
    }
    return meta;
}



static int romfs_open(void * opaque, const char * path, int flags, int mode) {
    const uint8_t * file;
    uint8_t is_directory;
    uint32_t datasize;
    int r = -1;

    file = romfs_get_address_by_path(opaque, path, &is_directory, &datasize);

    if(!file)
        return -1;
    if(is_directory)
        return -1;

    r = fio_open(romfs_read, NULL, romfs_seek, NULL, NULL);
    if (r > 0) {
        romfs_fds[r].file = file;
        romfs_fds[r].cursor = 0;
        romfs_fds[r].size = datasize;
        fio_set_opaque(r, romfs_fds + r);
    }
    return r;
}

void write_list_output(const uint8_t * romfs, uint32_t _max, char * output){
    
    const uint8_t * meta ; 
    int i, j;
    uint32_t output_index = 0;
    uint32_t max = _max;

    for(i = 0; max--; i++ ){
        meta = romfs + i * index_block_size + offset_filename;
        for(j = 0; j < filename_size && meta[j] ;++j ){
            output[output_index++] = meta[j]; 
        }
        output[output_index++] = '\n';
        output[output_index++] = '\r';
    }
    output[output_index++] = '\0';
}

static int romfs_list(void * opaque, const char * path, char * output) {
    const uint8_t * meta; 

    uint32_t datasize;
    uint8_t  is_directory = TRUE;


    meta = romfs_get_address_by_path(opaque, path, &is_directory, &datasize);
    if(!meta)
        return -1;
    if(!is_directory)
        return -1;

    //list the file name in directory
    write_list_output(meta, datasize, output);
    return 0;
}


void register_romfs(const char * mountpoint, const uint8_t * romfs) {
//    DBGOUT("Registering romfs `%s' @ %p\r\n", mountpoint, romfs);
    register_fs(mountpoint, romfs_open, romfs_list ,(void *) romfs);
}
