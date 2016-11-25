#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <string>
#include <errno.h>
#include "zlib.h"  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

//http://www.moon-soft.com/program/FORMAT/comm/tar.htm

/*
 * standard archive format - standard tar - ustar
 */
#define  recordsize  512
#define  namsiz      100
#define  tunmlen      32
#define  tgnmlen      32

union record {
    char        charptr[recordsize];
    struct header {
        char    name[namsiz];
        char    mode[8];
        char    uid[8];
        char    gid[8];
        char    size[12];
        char    mtime[12];
        char    chksum[8];
        char    linkflag;
        char    linkname[namsiz];
        char    magic[8];
        char    uname[tunmlen];
        char    gname[tgnmlen];
        char    devmajor[8];
        char    devminor[8];
        char    prefix[155];
    	char    padding[12];
    } header;
};
/* the checksum field is filled with this while the checksum is computed. */
#define    chkblanks    "        "        /* 8 blanks, no null */
/* the magic field is filled with this if uname and gname are valid. */
#define    tmagic    "ustar  "        /* 7 chars and a null */
/* the magic field is filled with this if this is a gnu format dump entry */
#define    gnumagic  "gnutar "        /* 7 chars and a null */
/* the linkflag defines the type of file */
#define  lf_oldnormal '\0'       /* normal disk file, unix compatible */
#define  lf_normal    '0'        /* normal disk file */
#define  lf_link      '1'        /* link to previously dumped file */
#define  lf_symlink   '2'        /* symbolic link */
#define  lf_chr       '3'        /* character special file */
#define  lf_blk       '4'        /* block special file */
#define  lf_dir       '5'        /* directory */
#define  lf_fifo      '6'        /* fifo special file */
#define  lf_contig    '7'        /* contiguous file */
/* further link types may be defined later. */
/* bits used in the mode field - values in octal */
#define  tsuid    04000        /* set uid on execution */
#define  tsgid    02000        /* set gid on execution */
#define  tsvtx    01000        /* save text (sticky bit) */
/* file permissions */
#define  turead   00400        /* read by owner */
#define  tuwrite  00200        /* write by owner */
#define  tuexec   00100        /* execute/search by owner */
#define  tgread   00040        /* read by group */
#define  tgwrite  00020        /* write by group */
#define  tgexec   00010        /* execute/search by group */
#define  toread   00004        /* read by other */
#define  towrite  00002        /* write by other */
#define  toexec   00001        /* execute/search by other */

/* GNU tar extensions */

#define GNUTYPE_DUMPDIR  'D'    /* file names from dumped directory */
#define GNUTYPE_LONGLINK 'K'    /* long link name */
#define GNUTYPE_LONGNAME 'L'    /* long file name */
#define GNUTYPE_MULTIVOL 'M'    /* continuation of file from another volume */
#define GNUTYPE_NAMES    'N'    /* file name that does not fit into main hdr */
#define GNUTYPE_SPARSE   'S'    /* sparse file */
#define GNUTYPE_VOLHDR   'V'    /* tape/volume header */

#include <sys/types.h>
#include <dirent.h>      
#include <unistd.h>
#include <grp.h>
#include <pwd.h>

uint32_t crc32_2(const uint8_t * data, uint32_t size)
{
    static uint32_t table[255] = {0};
    static uint8_t init = 0;
    
    const uint32_t poly = 0xEDB88320;
    const uint32_t begin = 0xFFFFFFFF;

    uint32_t i = 0;
    uint32_t reg = begin;

    if (!init) {
        init = 1;
        uint8_t j = 0;
        for (i = 0; i < 256; i++) {
            uint32_t crc = i;
            for (j = 0; j < 8; j++) {
                if (crc & 0x01) {
                    crc = (crc >> 1) ^ poly;
                } else {
                    crc >>= 1;
                }
            }
            table[i] = crc;
        }
    }

    for (i = 0; i < size; i++) {
        reg = table[(reg & 0xFF) ^ data[i]] ^ (reg >> 8);
    }

    return ~reg;
}

int checksum(const char * data, int size)
{
    int sum = 0;

    for (int i = 0; i < size; i++) {
        sum += data[i];
    }

    return sum;
}

static const int zoutbuf_size = recordsize * 2;
static z_stream zstream;
static unsigned char zoutbuf[zoutbuf_size];

ssize_t writen(int ofd, const void * buf, ssize_t size)
{
    const char * p = (const char *)buf;

    while (size > 0) {
        int writen = ::write(ofd, p, size);
        size -= writen;
        p += writen;
    }

    return size;
}

ssize_t write(int ofd, record * rec, bool compress, int flush = Z_NO_FLUSH)
{
    if (compress) {
        int remain = recordsize;
        int outlen = 0;
        int retwrtn = 0;
        int ret = Z_OK;

        while (remain > 0 || (flush == Z_FINISH && ret != Z_STREAM_END)) {
            unsigned char * next_in = (Bytef*)rec + recordsize - remain;
            unsigned char * next_out = (Bytef*)zoutbuf + outlen;
            
            zstream.next_in     = next_in;
            zstream.avail_in    = remain;
            zstream.next_out    = next_out;
            zstream.avail_out   = zoutbuf_size - outlen;

            ret = deflate(&zstream, flush);
            if (ret == Z_STREAM_ERROR) {
                printf("compress error!\n");
                exit(0);
            }

            remain -= zstream.next_in - next_in;
            outlen += zstream.next_out - next_out;

            if (outlen == zoutbuf_size) {
                retwrtn += writen(ofd, zoutbuf, outlen);
                outlen = 0;
            }
        } 

        if (outlen > 0) {
            retwrtn += writen(ofd, zoutbuf, outlen);
        }
        
        return retwrtn;
    } else {
        return writen(ofd, (const void *)rec, recordsize);
    }
}

void fill(record * rec, struct stat * sb)
{
    snprintf(rec->header.mode, 8, "%07o", sb->st_mode & 0777);
    snprintf(rec->header.mtime, 12, "%011lo", sb->st_mtime);

    if (sb->st_mode & S_IFREG) {
        snprintf(rec->header.size, 12, "%011lo", sb->st_size);
    } else {
        snprintf(rec->header.size, 12, "%011o", 0);
    }

    struct passwd * pd = getpwuid(sb->st_uid);
    if (pd != NULL) {
        snprintf(rec->header.uid, 8, "%07o", sb->st_uid);
        strcpy(rec->header.uname, pd->pw_name);
    }

    struct group * grp = getgrgid(sb->st_gid);
    if (grp != NULL) {
        snprintf(rec->header.gid, 8, "%07o", sb->st_gid);
        
        strcpy(rec->header.gname, grp->gr_name);
    }

    if (pd != NULL && grp != NULL) {
        strcpy(rec->header.magic, tmagic);
    }

    strncpy(rec->header.chksum, chkblanks, 8);
}

int longparams(int ofd, const char * name, int type, bool compress)
{
    {   //header
        record rec;
        memset(&rec, 0, sizeof(rec));

        strcpy(rec.header.name, "././@LongLink");
        snprintf(rec.header.mode, 8, "%07o", 0644);
        snprintf(rec.header.uid, 8, "%07o", 0);
        snprintf(rec.header.gid, 8, "%07o", 0);
        snprintf(rec.header.size, 12, "%011lo", strlen(name));
        snprintf(rec.header.mtime, 12, "%011o", 0);
        strcpy(rec.header.magic, tmagic);
        strcpy(rec.header.uname, "root");
        strcpy(rec.header.gname, "root");
        strncpy(rec.header.chksum, chkblanks, 8);
        
        rec.header.linkflag = type;
        
        uint32_t crc = checksum((const char *)&rec, recordsize);
        snprintf(rec.header.chksum, 7, "%06o", crc);
        rec.header.chksum[7] = ' ';

        write(ofd, &rec, compress);
    }

    {
        record rec;
        int size = strlen(name);
        const char * p = name;
        
        while (size > 0) {
            int cpy = size > recordsize ? recordsize : size;
            
            memset(&rec, 0, sizeof(rec));
            memcpy(rec.charptr, p, cpy);

            size -= cpy;
            p += cpy;

            write(ofd, &rec, compress);
        }
    }
}

int link(const char * name, int ofd, bool compress)
{
    printf("link:%s\n", name);
    
    if (1) {
        struct stat sb;
        if (stat(name, &sb) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
        }
        
        record rec;
        memset(&rec, 0, sizeof(record));
        
        fill(&rec, &sb);

        rec.header.linkflag = lf_symlink;

        char buf[4096];
        int linkname = 0;
        if ((linkname = readlink(name, buf, 4096)) <= 0) {
            printf("readlink(%s) error!%s\n", name, strerror(errno));
            exit(0);
        }

        buf[linkname] = '\0';

        if (linkname >= namsiz) {
            strncpy(rec.header.linkname, buf, namsiz);
            longparams(ofd, buf, GNUTYPE_LONGLINK, compress);
        } else {
            strncpy(rec.header.linkname, buf, linkname);
        }

        if (strlen(name) >= namsiz) {
            strncpy(rec.header.name, name, namsiz);
            longparams(ofd, name, GNUTYPE_LONGNAME, compress);
        } else {
            strcpy(rec.header.name, name);
        }

        uint32_t crc = checksum((const char *)&rec, recordsize);
        snprintf(rec.header.chksum, 7, "%06o", crc);
        rec.header.chksum[7] = ' ';

        write(ofd, &rec, compress);
        
    }
    return 0;
}

int file(const char * name, int ofd, bool compress)
{   
    printf("file:%s\n", name);

    if (1) {
        struct stat sb;
        if (stat(name, &sb) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
        }
        
        record rec;
        memset(&rec, 0, sizeof(record));
        
        fill(&rec, &sb);

        rec.header.linkflag = lf_normal;
        
        if (strlen(name) >= namsiz) {
            strncpy(rec.header.name, name, namsiz);
            longparams(ofd, name, GNUTYPE_LONGNAME, compress);
        } else {
            strcpy(rec.header.name, name);
        }

        uint32_t crc = checksum((const char *)&rec, recordsize);
        snprintf(rec.header.chksum, 7, "%06o", crc);
        rec.header.chksum[7] = ' ';

        write(ofd, &rec, compress);

        if (sb.st_size > 0) {
            int ifd = open(name, O_RDONLY);
            if (ifd < 0) {
                printf("open (%s) failed! %s\n", name, strerror(errno));
                exit(0);
            }
            
            while (1) {
                memset(&rec, 0, sizeof(record));
                if (read(ifd, (void *)&rec, recordsize) > 0) {
                    write(ofd, &rec, compress);
                } else {
                    break;
                }
            }
            close(ifd);
        }
        
    }
    
    return 0;
}

int directory(const char *name, int ofd, bool compress)
{
    printf("dire:%s\n", name);
    
    DIR * dir = opendir(name);
    if (dir == NULL) {
        printf("opendir(%s) failed! error:%s\n", name, strerror(errno));
        return -1;
    }

    if (1) {
        struct stat sb;
        if (stat(name, &sb) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
        }
        
        record rec;
        memset(&rec, 0, sizeof(record));
        
        fill(&rec, &sb);

        rec.header.linkflag = lf_dir;
        
        if (strlen(name) >= namsiz) {
            strncpy(rec.header.name, name, namsiz);
            longparams(ofd, name, GNUTYPE_LONGNAME, compress);
        } else {
            strcpy(rec.header.name, name);
        }

        uint32_t crc = checksum((const char *)&rec, recordsize);
        snprintf(rec.header.chksum, 7, "%06o", crc);
        rec.header.chksum[7] = ' ';

        write(ofd, &rec, compress);
    }

    struct dirent * entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        std::string sub(name);
        sub.append("/").append(entry->d_name);
        
        switch (entry->d_type) {
            case DT_DIR:  
                directory(sub.c_str(), ofd, compress);
                break;
            case DT_LNK:  
                link(sub.c_str(), ofd, compress);
                break;
            case DT_REG:    
                file(sub.c_str(), ofd, compress);
                break;
            default:       
                printf("unknown?\n");                
                break;
        }
    }

    closedir(dir);

    return 0;
}

int main(int argc, char * argv[])
{
    const char * outfile = NULL;
    const char * compresstype = NULL;
    int compression = Z_DEFAULT_COMPRESSION;
    const char * path = NULL;

    int ch;
    while((ch = getopt(argc, argv, "i:o:r:t:")) != -1) {
        switch(ch) {
            case 't': 
                compresstype = optarg;
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'i':
                path = optarg;
                break;
            case 'r':
                compression = atoi(optarg);
                if (compression > 10 || compression < 0) {
                    compression = Z_DEFAULT_COMPRESSION;
                }
                break;
            default:
                printf("unknown arguments. (%c)", ch);
                break;
        }
    }

    if (outfile == NULL) {
        printf("input outfile!\n");
        exit(0);
    }

    if (path == NULL) {
        printf("input compress file or directory!\n");
        exit(0);
    }

    enum en_type {
        en_no_compress,
        en_zip,
        en_flate,
    };
    
    int type = en_no_compress;
    if (compresstype != NULL) {
        if (!strcmp(compresstype, "zip")) {
            type = en_zip;
        } else if (!strcmp(compresstype, "flate")) {
            type = en_flate;
        } 
    }

    printf("compress type:%d\n", type);
    
    int fd = open(outfile, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    
    if (type != en_no_compress) {
        memset(&zstream, 0, sizeof(zstream));
        
        zstream.zalloc  = NULL;
        zstream.zfree   = NULL;
        zstream.opaque  = NULL;

        if (type == en_zip) {
            int ret = deflateInit2(&zstream, compression, 
                Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);;
        } else if (type == en_flate) {
            int ret = deflateInit(&zstream, compression);
        }
    }
    
    directory(path, fd, (type != en_no_compress));

    record rec = {0};
    write(fd, &rec, (type != en_no_compress));
    write(fd, &rec, (type != en_no_compress), Z_FINISH);
        
    close(fd);

    if (type != en_no_compress) {
        deflateEnd(&zstream);
    }
    
    return 0;
}
