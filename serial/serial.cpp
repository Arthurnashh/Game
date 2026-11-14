#include <stdio.h>
#include <windows.h>

#define START   10000000
#define LIMIT   99999999

#include "serial.hpp"

int main(int argc, char *argv[])
{
	srand(GetTickCount());
	FILE *f=NULL;
	if( argc>=2 && argv[1] ) f=fopen(argv[1],"wt");
	unsigned long i, total;
	total = 0;
	for (i = START; i < LIMIT; i++)
	{
		if (CheckCode(i) == SERIAL_KEY)
		{
			if( rand()>RAND_MAX/10 ) continue;
			if( f ) fprintf(f,"%d\n", i);
			else printf("%d\n", i);
			total++;
		}
	}
	printf
	(
		"%d keys found out of %d tested.  %f percent.\n",
		total, LIMIT,
		100.0 * total/(float)LIMIT
	);

	if( f ) fclose(f);

	return(0);
}

