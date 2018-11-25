#include <stdio.h>
#include "t_writefile.h"

int t_writefile(DataFile_ypeDef* pReadStruct)
{
    FILE* fp;

    fp = fopen(pReadStruct->FileName, "wb+");
    if(!fp)
    {
        return -1;
    }
    if(pReadStruct->FileSize < 0)
    {
        return -2;
    }
    if(pReadStruct->pFileData == NULL)
    {
        return -4;
    }
    fwrite(pReadStruct->pFileData, sizeof(unsigned char), pReadStruct->FileSize, fp);
    fclose(fp);

    return 0;
}
