#include <stdio.h>
#include "foo.h"

int main(void)
{
	printf("bar = %d\n", bar);
	foo();
	printf("bar = %d\n", bar);

	return 0;
}
