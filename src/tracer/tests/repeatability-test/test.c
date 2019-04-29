

unsigned long fibonacci(unsigned long curr, unsigned long prev) {

  return prev + curr;
}

void main() {
  int i = 1;
  unsigned long prev = 0, curr = 1;
  int delay = 1000;
  unsigned long ret = 1;
  while (1) {
    ret = fibonacci(curr, prev);
    prev = curr;
    curr = ret;
    i++;
  }

  while(1);
}
