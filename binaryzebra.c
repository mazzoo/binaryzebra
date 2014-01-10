#include <sys/types.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>


#define BUF_SZ 1024
char buf[BUF_SZ];

void usage(void)
{
  printf("usage:\n");
  printf("\tbinaryzebra n_files image [n_bytes]\n");
  printf("\n");
  printf("\tsplit an image into multiple files\n");
  printf("\te.g. with n_files=2 one file will get all odd bytes, the other every even byte\n");
  printf("\te.g. with n_files=4 file1 will get all 4th bytes,\n");
  printf("\t                    file2          all 4th+1 bytes\n");
  printf("\t                    file3          all 4th+2 bytes\n");
  printf("\t                    file4          all 4th+3 bytes\n");
  printf("\tetc.\n");
  printf("\tn_bytes is optional, and can be given dec or hex\n");
  printf("\n");
}

void usage_exit(char * err)
{
  printf("\nERROR: ");
  printf(err);
  printf("\n\n");
  usage();
  exit(1);
}


int main(int argc, char ** argv)
{
  int ret = 0;

  if ( (argc < 3) || (argc > 4) )
  {
    usage_exit("two or three arguments required");
  }

  int fimage;
  int fsize;
  uint8_t * image;

  fimage = open(argv[2], O_RDONLY);
  if (fimage < 0)
  {
    snprintf(buf, BUF_SZ, "cannot open file %s", argv[2]);
    usage_exit(buf);
  }
  fsize = lseek(fimage, 0, SEEK_END);
  if (fimage < 0)
  {
    snprintf(buf, BUF_SZ, "cannot seek file %s", argv[2]);
    usage_exit(buf);
  }
  ret = lseek(fimage, 0, SEEK_SET);
  if (ret < 0)
  {
    snprintf(buf, BUF_SZ, "cannot rewind file %s", argv[2]);
    usage_exit(buf);
  }

  image = malloc(fsize);
  if (!image)
  {
    usage_exit("not enough memory");
  }

  ret = read(fimage, image, fsize);
  if (ret != fsize)
  {
    snprintf(buf, BUF_SZ, "read: expected %d but got %d", fsize, ret);
    usage_exit(buf);
  }
  close(fimage);


  char * end;
  int n_files;

  end = argv[1];
  n_files = strtoul(argv[1], &end, 0);
  if ((errno == ERANGE && (n_files == LONG_MAX || n_files == LONG_MIN))
      || (errno != 0 && n_files == 0))
  {
    usage_exit("cannot parse your n_files");
  }
  if (end == argv[1])
  {
    usage_exit("cannot parse your n_files");
  }
  if (n_files >= fsize)
  {
    usage_exit("n_files bigger than filesize");
  }
  if (n_files < 2)
  {
    usage_exit("n_files less than 2; use cp instead");
  }

  int n_bytes = 1;

  if (argc > 3)
  {
    end = argv[3];
    n_bytes = strtoul(argv[3], &end, 0);
    if ((errno == ERANGE && (n_bytes == LONG_MAX || n_bytes == LONG_MIN))
        || (errno != 0 && n_bytes == 0))
    {
      usage_exit("cannot parse your n_bytes");
    }
    if (end == argv[3])
    {
      usage_exit("cannot parse your n_bytes");
    }
    if (n_bytes * n_files > fsize)
    {
      usage_exit("n_bytes * n_files bigger than filesize");
    }
  }

  /* open output files */
  int * output;
  output = malloc(n_files * sizeof(*output));
  if (!output)
  {
    usage_exit("not enough memory for output file descriptors");
  }

  int i;
  for (i = 0; i < n_files; i++)
  {
    snprintf(buf, BUF_SZ, "zebra_0x%x_0x%x_0x%.8x.bin", n_files, n_bytes, i);
    output[i] = open(buf, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
    if (output[i] < 0)
    {
      printf("cannot open %s for writing", buf);
      usage_exit(NULL);
    }
  }

  /* dump */
  for (i = 0; i < fsize; i += n_bytes * n_files)
  {
    int j;
    for (j = 0; j < n_files; j++)
    {
      ret = write(output[j], &image[i+j*n_bytes], n_bytes);
      if (ret != n_bytes)
      {
        snprintf(buf, BUF_SZ, "cannot write file %d - wanted to write %d but only wrote %d", j, n_bytes, ret);
        usage_exit(buf);
      }
    }
  }

  /* close */
  for (i = 0; i < n_files; i++)
  {
    close(output[i]);
  }

  return 0;
}
