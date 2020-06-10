//
// Parse 70mai MP4, extract GPS data to SRT
// by freezer52000 mod dexterxx.pl
//
// compile: gcc maigpsmass.c -o maigpsmass
// use:     maigpsmass directory/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/types.h>
#include <dirent.h>

struct gx {
	int f1;
	int f2;
	int f3;
	int speed;
	char N;
	int lat;
	char E;
	int lng;
} __attribute__((packed));

long b2l(unsigned char *b, int count)
{
	int i;
	long out=0;
	for (i = 0; i < count; i++) {
		out <<= 8;
		out |= b[i];
	}
	return out;
}

int extractFile(const char *filename)
{
	int fd, ft, i, mp, pos;
	long sz;
	char *b;
	struct gx *g;
	char name[5];
	char targetFilename[80];
	strcpy(targetFilename, filename);
	strcat(targetFilename, ".srt");

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}
	b = malloc(36*120*2);
	
	ft = open(targetFilename, O_WRONLY | O_CREAT, 0644);
	if (ft < 0) {
		perror("open to write");
		return -1;
	}
	
	if (dup2(ft, 1) == -1) {
		perror("dup2 failed"); 
		return -1;
	}

	pos = 0;
next:
	lseek(fd, pos, SEEK_SET);
	read(fd, b, 8);
	memcpy(name, b+4, 4);
	name[4] = 0;
	sz = b2l((void *)b, 4);
	if (sz == 1) {
		lseek(fd, pos+8, SEEK_SET);
		read(fd, b, 8);
		sz = b2l((void *)b, 8);
	}
	pos += sz;
	if (strcmp(name, "GPS ") != 0)
		goto next;

	read(fd, b, 36*120*2);
	close(fd);
	mp = sz / 36;

	for (i = 0; i < mp; i++) {
		g = (void *)(b + i * 36);
		if (g->N != 'N' && g->N != 'S')
			continue;

		printf("%d\n", g->f3+1);
		printf("00:%02d:%02d,000 --> 00:%02d:%02d,999\n", g->f3/60, g->f3%60, g->f3/60, g->f3%60);
		printf("%c%d.%05d %c%d.%05d\n", 
			g->N, g->lat/100000, (int)((((long)g->lat%100000)*100000)/60000), 
			g->E, g->lng/100000, (int)((((long)g->lng%100000)*100000)/60000));
		printf("%d.%03d km/h\n", g->speed/1000, g->speed%1000);
		printf("\n");
	}
	free(b);
	fsync(ft);
	fflush(NULL);
	close(ft);
	return 0;
}

int endsWith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int main(int argc, char *argv[])
{
    DIR *dp;
    struct dirent *ep;
    dp = opendir(argv[1]);

    if (dp != NULL)
    {
        while (ep = readdir(dp))
            if (endsWith(ep->d_name, ".MP4") || endsWith(ep->d_name, ".mp4"))
                extractFile(ep->d_name);

    	(void) closedir(dp);
    }
    else
    	perror("Couldn't open the directory");

    return 0;
}
