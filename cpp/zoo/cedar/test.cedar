extern void* stdout;
extern int fprintf(void* stream, const char* format, ...);

int
main(int argc, char* argv[])
{
  for (int i = 0; i < argc; i++) {
    fprintf(stdout, "[%d] = %s\n", i, argv[i]);
  }
  return 0;
}
