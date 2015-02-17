#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ftw.h>
#include <fcntl.h>

char *newhdr;
int newhdr_size;

const char *skipws(const char *s)
{
  while(*s == 10 || *s == 13 || *s == 32 || *s == 8)
    s++;
  return s;
}



static void
writeout(const char *src, int size, const char *path, int pragmaonce)
{
  int fd = open(path, O_WRONLY | O_TRUNC);
  if(fd == -1) {
    perror("open");
    exit(1);
  }

  if(write(fd, newhdr, newhdr_size) != newhdr_size) {
    perror("write");
    exit(1);
  }

  if(pragmaonce) {
    const char *str = "#pragma once\n";
    if(write(fd, str, strlen(str)) != strlen(str)) {
      perror("write");
      exit(1);
    }
  }

  if(write(fd, src, size) != size) {
    perror("write");
    exit(1);
  }
  close(fd);
}


static void
process_buf(char *buf, int size, const char *path)
{
  const char *s = buf;
  int have_pragma_once = 0;
  if(!strncmp(s, "#pragma once", strlen("#pragma once"))) {
    have_pragma_once = 1;
    s += strlen("#pragma once");
  }

  s = skipws(s);

  if(s[0] == '/' && s[1] == '*') {
    // Begin of block
    s+= 2;
    while(1) {
      if(*s == 0)
        return;
      if(s[0] == '*' && s[1] == '/') {
        s+= 2;
        break;
      }
      s++;
    }
  }

  s = skipws(s);
  if(!strncmp(s, "#pragma once", strlen("#pragma once"))) {
    have_pragma_once = 1;
    s += strlen("#pragma once");
  }

  s = skipws(s);

  #if 0
  printf("%s %sstarts here\n%s", path,
         have_pragma_once ? "(with #pragma once) " : "", s);
  #endif



  writeout(s, size - (s - buf), path, have_pragma_once);

}

static void
process_file(const char *path, const struct stat *st)
{
  printf("Processing %s (%zd) bytes\n", path, st->st_size);

  char *buf = malloc(st->st_size + 1);
  if(buf == NULL)
    return;
  buf[st->st_size] = 0;
  int fd = open(path, O_RDONLY);
  if(fd == -1) {
    perror("open");
    exit(1);
  }

  if(read(fd, buf, st->st_size) != st->st_size) {
    perror("read");
    exit(1);
    return;
  }

  close(fd);

  process_buf(buf, st->st_size, path);
  free(buf);
}


static int
cb(const char *path, const struct stat *ptr, int flag, struct FTW *f)
{
  if(flag != FTW_F)
    return 0;

  const char *postfix = strrchr(path, '.');
  if(postfix == NULL)
    return 0;
  postfix++;
  if(!strcmp(postfix, "h") ||
     !strcmp(postfix, "cpp") ||
     !strcmp(postfix, "cc") ||
     !strcmp(postfix, "m"))
    process_file(path, ptr);
  return 0;
}


static void
load_new_header(const char *path)
{
  struct stat st;
  int fd = open(path, O_RDONLY);
  if(fd == -1) {
    perror(path);
    exit(1);
  }
  if(fstat(fd, &st)) {
    perror(path);
    exit(1);
  }

  if((newhdr = malloc(st.st_size)) == NULL) {
    perror(path);
    exit(1);
  }

  if(read(fd, newhdr, st.st_size) != st.st_size) {
    perror(path);
    exit(1);
  }

  close(fd);

  newhdr_size = st.st_size;
}


int
main(int argc, char **argv)
{
  if(argc < 3)
    exit(0);

  load_new_header(argv[1]);

  nftw(argv[2], cb, 1000, FTW_PHYS | FTW_MOUNT);
  return 0;
}
