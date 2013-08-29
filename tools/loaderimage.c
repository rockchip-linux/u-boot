#include "compiler.h"



#define LOADER_MAGIC_SIZE     16
#define LOADER_HASH_SIZE      32
#define CMD_LINE_SIZE         512
#define MODE_PACK             0
#define MODE_UNPACK           1
#define UBOOT_NUM             4
#define UBOOT_MAX_SIZE        1024*1024
typedef struct tag_second_loader_hdr
{
    unsigned char magic[LOADER_MAGIC_SIZE];  // "LOADER  "
    
    unsigned int loader_load_addr;           /* physical load addr ,default is 0x60000000*/
    unsigned int loader_load_size;           /* size in bytes */
    unsigned int crc32;                      /* crc32 */
    unsigned int hash_len;                   /* 20 or 32 , 0 is no hash*/
    
    unsigned char hash[LOADER_HASH_SIZE];     /* sha */
    unsigned char rsahash[LOADER_HASH_SIZE];  /* asciiz product name */
    
    unsigned char cmdline[CMD_LINE_SIZE];    
    
    unsigned char reserved[1024-512-96];
}second_loader_hdr;

void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s [--pack|--unpack] file\n", prog);
}

int main (int argc, char *argv[])
{
	int	mode, size, i;
	FILE	*fi, *fo;
	second_loader_hdr hdr;
	char *buf = 0;

	if (argc < 4) {
		usage(argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!strcmp(argv[1], "--pack"))
		mode = MODE_PACK;
	else if (!strcmp(argv[1], "--unpack"))
		mode = MODE_UNPACK;
	else {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	fi = fopen(argv[2], "rb");
	if (!fi) {
		perror(argv[2]);
		exit (EXIT_FAILURE);
	}

	fo = fopen(argv[3], "wb");
	if (!fo) {
		perror(argv[3]);
		exit (EXIT_FAILURE);
	}
	
	buf = calloc(UBOOT_MAX_SIZE, UBOOT_NUM);
	if (!buf) {
		perror(argv[3]);
		exit (EXIT_FAILURE);
	}
	//memset(buf, 0, UBOOT_NUM*UBOOT_MAX_SIZE);
	if(mode == MODE_PACK){
		printf("pack input %s \n", argv[2]);
		fseek( fi, 0, SEEK_END );
		size = ftell(fi);
		fseek( fi, 0, SEEK_SET );
		printf("pack file size:%d \n", size);
		if(size > UBOOT_MAX_SIZE - sizeof(second_loader_hdr)){
			perror(argv[3]);
			exit (EXIT_FAILURE);
		}
		memset(&hdr, 0, sizeof(second_loader_hdr));
		strcpy(hdr.magic,"LOADER");
		hdr.loader_load_addr = 0x60000000;
		hdr.loader_load_size = size;
		hdr.crc32 = 0;
		memcpy(buf, &hdr, sizeof(second_loader_hdr));
		fread(buf + sizeof(second_loader_hdr), size, 1, fi);
		for(i=0; i<UBOOT_NUM; i++){
			fwrite(buf, UBOOT_MAX_SIZE, 1, fo);
		}
		printf("pack %s success! \n", argv[3]);
	}else if (mode == MODE_UNPACK){
		printf("unpack input %s \n", argv[2]);
		memset(&hdr, 0, sizeof(second_loader_hdr));
		fread(&hdr, sizeof(second_loader_hdr), 1, fi);
		fread(buf, hdr.loader_load_size, 1, fi);
		fwrite(buf, hdr.loader_load_size, 1, fo);
		printf("unpack %s success! \n", argv[3]);
	}
	free(buf);
	fclose(fi);
	fclose(fo);

}

