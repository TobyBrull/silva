typedef void FILE;
extern FILE* stdout;
extern int fprintf(FILE* __restrict __stream, const char* __restrict __format, ...);

int
main(int argc, char* argv[])
{
  for (int i = 0; i < argc; i++) {
    fprintf(stdout, "[%d] = %s\n", i, argv[i]);
  }
  return 0;
}
