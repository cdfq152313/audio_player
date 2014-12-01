#include "osdebug.h"
#include "filesystem.h"
#include "fio.h"

#include <stdint.h>
#include <string.h>
#include <hash-djb2.h>

#define MAX_FS 16

struct fs_t {
    uint32_t hash;
    const char * mountpoint;
    fs_open_t cb;
    fs_list_t list;
    void * opaque;
};

static struct fs_t fss[MAX_FS];

__attribute__((constructor)) void fs_init() {
    memset(fss, 0, sizeof(fss));
}

int register_fs(const char * mountpoint, fs_open_t callback, fs_list_t list, void * opaque) {
    int i;
    DBGOUT("register_fs(\"%s\", %p, %p)\r\n", mountpoint, callback, opaque);
    
    for (i = 0; i < MAX_FS; i++) {
        if (!fss[i].cb) {
            fss[i].hash = hash_djb2((const uint8_t *) mountpoint, 0, -1);
            fss[i].mountpoint = mountpoint;
            fss[i].cb = callback;
            fss[i].list = list;
            fss[i].opaque = opaque;
            return 0;
        }
    }
    
    return -1;
}

int fs_list_root(char * output){
    int i;
    
    output[0] = '\0';
    for(i = 0; i < MAX_FS; ++i){
        if(fss[i].cb){
            strcat(output, fss[i].mountpoint);
            strcat(output, "\n");
        }
    }
    return 0;
}

int fs_list_mountpoint(const char * path, char * output){
    const char * slash;
    uint32_t hash;
    int i;

    slash = strchr(path, '/');

    if(slash)
        hash = hash_djb2((const uint8_t*) path, 0, slash - path);
    else
        hash = hash_djb2((const uint8_t*) path, 0, -1);

    for(i = 0; i < MAX_FS; i++){
        if(fss[i].hash == hash){
            if(slash)
                return fss[i].list(fss[i].opaque, slash+1, output);
            else
                return fss[i].list(fss[i].opaque, "/", output);
        }
    }
    return -1;
}

int fs_list(const char * path, char * output){
    
    //if only ///// list root
    while(path[0] == '/')
        path++;
    if(path[0] == '\0'){
        return fs_list_root(output);
    }

    //else list mountpoint
    else{
        return fs_list_mountpoint(path, output);
    }
}

int fs_open(const char * path, int flags, int mode) {
    const char * slash;
    uint32_t hash;
    int i;
//    DBGOUT("fs_open(\"%s\", %i, %i)\r\n", path, flags, mode);
    
    while (path[0] == '/')
        path++;
    
    slash = strchr(path, '/');
    
    if (!slash)
        return -2;

    hash = hash_djb2((const uint8_t *) path, 0,slash - path);
    path = slash + 1;

    for (i = 0; i < MAX_FS; i++) {
        if (fss[i].hash == hash)
            return fss[i].cb(fss[i].opaque, path, flags, mode);
    }
    
    return -2;
}
