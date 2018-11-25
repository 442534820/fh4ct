#include <sys/stat.h>
#include <stdio.h>

long t_filesize(char* FileName)
{
    FILE* fp;
    long FileSize = -1;
    struct stat StatBuff;
    fp = fopen(FileName, "r");
    if(!fp)
        return -1;
    if(stat(FileName, &StatBuff) < 0)
        return -1;
    FileSize = StatBuff.st_size;
    fclose(fp);
    return FileSize;
}
