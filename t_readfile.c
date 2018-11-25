#include <sys/stat.h>
#include <stdio.h>
#include "t_readfile.h"

int t_readfile(DataFile_ypeDef* pReadStruct)
{
    FILE* fp;

    fp = fopen(pReadStruct->FileName, "r");
    if(!fp)
    {
        return -1;
    }
    pReadStruct->FileSize = t_filesize(pReadStruct->FileName);
    if(pReadStruct->FileSize < 0)
    {
        return -2;
    }
    pReadStruct->pFileData = (unsigned char *)malloc(pReadStruct->FileSize);
    if(pReadStruct->pFileData == NULL)
    {
        return -3;
    }
    fread(pReadStruct->pFileData, sizeof(unsigned char), pReadStruct->FileSize, fp);
    fclose(fp);

    return 0;
}
