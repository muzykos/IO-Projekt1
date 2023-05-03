#include <stdio.h>
int main(int argc, char *argv[]) {
        char *filename;
        FILE *fp;
        if (argc > 1)
        {
            fp = fopen(argv[1], "w");
        }else{
            fp = fopen("myfile", "w");
        }
        int X = 2048 * 1024 - 1;
        fseek(fp, X , SEEK_SET);
        fputc('\0', fp);
        fclose(fp);
}