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
#include <map>
#include <sys/types.h>
#include <utime.h>

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

enum en_file_type
{
    en_unkown_file,
    en_gzip_file,
    en_tar_file,
};

enum en_decompress_stage
{
    en_decompress_invalid,
    en_decompress_header,
    en_decompress_longname,
    en_decompress_longlink,
    en_decompress_data,
};

int  gzdecompress(Byte  *zdata, uLong nzdata, 
                 Byte  *data, uLong *ndata) 
{ 
    int  err = 0; 
    z_stream d_stream = {0};  /* decompression stream */
    
    static   char  dummy_head[2] = { 
        0x8 + 0x7 * 0x10, 
        (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF, 
    }; 
    
    d_stream.zalloc     = NULL; 
    d_stream.zfree      = NULL; 
    d_stream.opaque     = NULL; 
    d_stream.next_in    = zdata; 
    d_stream.avail_in   = 0; 
    d_stream.next_out   = data; 
    
    if (inflateInit2(&d_stream, MAX_WBITS + 16) != Z_OK)  return  -1; 
    //if(inflateInit2(&d_stream, 47) != Z_OK) return -1;  
    while (d_stream.total_out < *ndata && d_stream.total_in < nzdata) { 
        
        d_stream.avail_in = d_stream.avail_out = 1;  /* force small buffers */  
        
        if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END)  
            break ; 
        if (err != Z_OK) { 
            if (err == Z_DATA_ERROR) { 
                d_stream.next_in = (Bytef*) dummy_head; 
                d_stream.avail_in =  sizeof (dummy_head); 
                
                if ((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK) { 
                    return  -1; 
                } 
                
            }  else  
                return  -1; 
        } 
    } 
    if (inflateEnd(&d_stream) != Z_OK)  return  -1; 
    *ndata = d_stream.total_out; 
    return  0; 
}

en_file_type check_filetype(const unsigned char * data, off_t size)
{
    if (size < 2) {
        return en_unkown_file;
    }

    if (data[0] == 0x1F && data[1] == 0x8B) {
        return en_gzip_file;
    }

    if (size >= recordsize) {
        record r;
        memcpy((void *)&r, (const void *)data, sizeof(record));

        if (strlen(r.header.uname) > 0 && strlen(r.header.gname) > 0) {
            if (strcmp(r.header.magic, tmagic) == 0) {
                return en_tar_file;
            }
        } else if (strcmp(r.header.magic, gnumagic) == 0) {
            return en_tar_file;
        }
    }

    return en_unkown_file;
}

bool check_crc(const record & r)
{
    return true;
}

int get_oct(const char * data, int len)
{
    int oct = 0;
    char c;

    while (len --) {
        c = *data ++;
        if (c == 0 || c == '\0') {
            break;
        }

        if (c == ' ') {
            continue;
        }

        if (c < '0' || c > '7') {
            oct = -1;
            break;
        }

        oct = oct * 8 + (c - '0');
    }
    
    return oct;
}

char *strtime (time_t *t)
{
      struct tm   *local;
      static char result[32];

      local = localtime(t);
      sprintf(result,"%4d/%02d/%02d %02d:%02d:%02d",
              local->tm_year+1900, local->tm_mon+1, local->tm_mday,
              local->tm_hour, local->tm_min, local->tm_sec);
      return result;
}

int makedir(const char * newdir)
{
    printf("make:%s\n", newdir);
    
    std::string path(newdir);
    std::string dir;

    path.append("/");

    std::string::size_type nbegin = 0;
    std::string::size_type npos   = 0;

    while ((npos = path.find('/', nbegin)) != std::string::npos) {
        dir.append(path.substr(nbegin, npos - nbegin));
        
        if ((mkdir(dir.c_str(), 0755) == -1) && (errno == ENOENT)) {
            printf("create director:%s failed!\n", dir.c_str());
            return -1;
        }

        nbegin = npos + 1;
        dir.append("/");
    }

    return 0;
}

int main(int argc, char * argv[])
{
    std::string ifile;
    std::string ofile;
    en_file_type filetype = en_unkown_file;
    std::string rpath;
    int interval = 0;
    
    int ch;
    while((ch = getopt(argc, argv, "i:o:c:t:")) != -1) {
        switch(ch) {
            case 'i':
                std::string(optarg).swap(ifile);
                break;
            case 'o':
                std::string(optarg).swap(ofile);
                break;
            case 'c':
                std::string(optarg).swap(rpath);
                break;
            case 't':
                interval = atoi(optarg);
                break;
            default:
                printf("unknown arguments. (%c)", ch);
                break;
        }
    }

    printf("input:%s, output:%s, rpath:%s\n", 
        ifile.c_str(), ofile.c_str(), rpath.c_str());

    if (access(ifile.c_str(), F_OK) != 0) {
        printf("invalid input file(%s)!\n", ifile.c_str());
        exit(0);
    }

    struct stat sb;
    if (stat(ifile.c_str(), &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if ((sb.st_mode & S_IFMT) != S_IFREG) {
        printf("(%s) not regular file!\n", ifile.c_str());
        exit(0);
    }

    off_t size = sb.st_size;
    if (size == 0) {
        printf("(%s) size zero!\n", ifile.c_str());
        exit(0);
    }

    int ifd = open(ifile.c_str(), O_RDONLY);
    if (ifd < 0) {
        printf("read file(%s) failed, %s\n", ifile.c_str(), strerror(errno));
        exit(0);
    }

    const off_t isize = 512, osize = 512;
    off_t remaining = 0;
    off_t outlen    = 0;
    off_t readn     = 0;
    
    unsigned char * ibuf, * obuf, * pbuf;
    ibuf = (unsigned char *)malloc(isize);
    obuf = (unsigned char *)malloc(osize);

    z_stream d_stream = {0};
    
    d_stream.zalloc     = NULL; 
    d_stream.zfree      = NULL; 
    d_stream.opaque     = NULL; 

    if (inflateInit2(&d_stream, MAX_WBITS + 16) != Z_OK) {
        printf("inflate init failed!\n");
        exit(0);
    }

    bool longlinkname = false;
    bool done = false;
    record header_;

    char    slink[4096];
    char    fname[4096];
    int     fmode;
    int     fsize;
    time_t  ftime;
    int     ofd;
    int     noff;
    std::map<std::string, time_t> ftimemap;

    en_decompress_stage stage = en_decompress_header;
    en_decompress_stage last_stage = en_decompress_invalid;

    if (!rpath.empty()) {
        rpath.append("/");
        makedir(rpath.c_str());
    }
    
    while (!done && (readn = read(ifd, ibuf, isize)) > 0) {

        printf("read %ld\n", readn);
        remaining = readn;

        if (filetype == en_unkown_file) {
            filetype = check_filetype(ibuf, readn);

            printf("filetype:%d\n", filetype);
            if (filetype == en_unkown_file) {
                printf("unkown file type!\n");
                break;
            }
        }
        
        while (!done && remaining > 0) {
            printf("remain:%ld, outlen:%ld\n", remaining, outlen);

            if (filetype == en_gzip_file) {
                unsigned char * next_in    = ibuf + readn - remaining;
                d_stream.avail_in = remaining; 
                d_stream.next_in  = next_in;

                unsigned char * next_out   = obuf + outlen;
                d_stream.avail_out = osize - outlen;
                d_stream.next_out  = next_out;

                int ret = inflate(&d_stream, Z_NO_FLUSH);
                if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT) {
                } 
                if (ret == Z_MEM_ERROR) {
                }
                if (ret == Z_DATA_ERROR) {
                }

                remaining -= d_stream.next_in - next_in;
                outlen    += d_stream.next_out - next_out;
                pbuf       = obuf;
            } else {
                outlen    = remaining;
                remaining = 0;
                pbuf      = ibuf;
            }

            printf("remain:%ld, outlen:%ld\n", remaining, outlen);

            //untar
            if (outlen == osize) {
                outlen = 0;
                
                switch (stage) {
                    case en_decompress_header: {
                        printf("en_decompress_header\n");
                        memcpy((void *)&header_, (const void *)pbuf, osize);

                        if (header_.header.name[0] == 0) {
                            printf("meet end of tar!\n");
                            done = true;
                            break;
                        }

                        if (!check_crc(header_)) {
                            printf("file(%s) crc check failed!\n", 
                                header_.header.name);
                            exit(0);
                        }

                        fmode = get_oct(header_.header.mode, 8);
                        ftime = get_oct(header_.header.mtime, 12);
                        fsize = get_oct(header_.header.size, 12);

                        if (fmode == -1 || ftime == (time_t)-1) {
                            printf("invalid file. fmode:%d, ftime:%ld\n", fmode, ftime);
                            exit(0);
                        }
                        
                        if (last_stage == en_decompress_longname) {
                            if (memcmp(fname, header_.header.name, namsiz - 1)) {
                                printf("bad long name!\n");
                                exit(0);
                            }
                        } else if (!longlinkname) {
                            strcpy(fname, header_.header.name);
                        }

                        printf("%s %d %o %s\n", strtime(&ftime), fsize, fmode, fname);
                        
                        switch (header_.header.linkflag) {
                            case lf_oldnormal:
                            case lf_normal: {

                                std::string file(rpath);
                                file.append(fname);

                                ofd = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, fmode);
                                if (ofd < 0) {
                                    printf("open file(%s) failed!(%s)\n", file.c_str(), strerror(errno));
                                    exit(0);
                                }
                                if (fsize > 0) {
                                    stage = en_decompress_data;
                                } else {
                                    stage = en_decompress_header;
                                }
                                ftimemap[file] = ftime;
                                
                                longlinkname = false;
                                break;
                            }
                            case lf_symlink: {
                                const char * oldpath = header_.header.linkname;
                                const char * newpath = header_.header.name;
                                printf("sym:%s, %s\n", oldpath, newpath);
                                printf("last stage:%d, longlink:%d\n", last_stage, longlinkname);
                                printf("slink:%s\n", slink);
                                
                                if (last_stage == en_decompress_longname) {
                                    newpath = fname;

                                    if (memcmp(newpath, header_.header.name, namsiz - 1)) {
                                        printf("bad link name!\n");
                                        exit(0);
                                    }
                                }

                                if (longlinkname || last_stage == en_decompress_longlink) {
                                    oldpath = slink;

                                    printf("oldpath:%s\n", oldpath);
                                    printf("linknam:%s\n", header_.header.linkname);

                                    if (memcmp(oldpath, &header_.header.linkname[0], namsiz - 1)) {
                                        printf("bad long name for link!\n");
                                        exit(0);
                                    }
                                }
                                
                                std::string newp(rpath);
                                newp.append(newpath);
                                printf("sym:%s, %s\n", oldpath, newpath);

                                symlink(oldpath, newp.c_str());

                                ftimemap[newp] = ftime;

                                longlinkname = false;
                                stage = en_decompress_header;
                                break;
                            }
                            case lf_dir: {
                                std::string dir(rpath);
                                dir.append(fname);
                                makedir(dir.c_str());
                                longlinkname = false;

                                ftimemap[dir] = ftime;
                                
                                stage = en_decompress_header;    
                                break;
                            }
                            case GNUTYPE_LONGLINK:
                                stage = en_decompress_longlink;
                                longlinkname = true;
                                noff = 0;
                                break;
                            case GNUTYPE_LONGNAME:
                                stage = en_decompress_longname;
                                noff  = 0;
                                break;
                        }
                        last_stage = en_decompress_header;
                        
                        break;
                    }
                    case en_decompress_data: {
                        printf("en_decompress_data\n");
                        int writen = fsize > osize ? osize : fsize;
                        write(ofd, pbuf, writen);
                        fsize -= writen;

                        if (fsize == 0) {
                            last_stage = stage;
                            stage = en_decompress_header;
                            close(ofd);
                            ofd = -1;
                        }
                        break;
                    } 
                    case en_decompress_longname: {
                        printf("en_decompress_longname\n");
                        if (last_stage != en_decompress_longname) {
                            fname[fsize] = '\0';
                        }
                        
                        int size = fsize > osize ? osize : fsize;
                        strncpy(fname + noff, (const char *)pbuf, size);

                        fsize -= size;
                        noff  += size;

                        last_stage = stage;
                        if (fsize == 0) {
                            noff = 0;
                            stage = en_decompress_header;
                        }
                        break;
                    }
                    case en_decompress_longlink: {
                        printf("en_decompress_longlink\n");
                        if (last_stage != en_decompress_longlink) {
                            slink[fsize] = '\0';
                        }
                        
                        int size = fsize > osize ? osize : fsize;
                        strncpy(slink + noff, (const char *)pbuf, size);

                        fsize -= size;
                        noff  += size;

                        last_stage = stage;
                        if (fsize == 0) {
                            noff = 0;
                            stage = en_decompress_header;
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            if (interval > 0) {
                sleep(interval);
            }
        }
        
    }

    struct utimbuf settime;
    for (std::map<std::string, time_t>::iterator it = ftimemap.begin();
        it != ftimemap.end(); ++ it) {

        settime.actime = settime.modtime = it->second;
        
        utime(it->first.c_str(), &settime);

        printf("chage:%s, %s\n", it->first.c_str(), strtime(&it->second));
    }

    if (inflateEnd(&d_stream) != Z_OK)  return  -1; 
    
    close(ifd);
    
    return 0;
}
