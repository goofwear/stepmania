// Ehhh....
//
// I think I'll just put this one in the public domain
// (with no warranty as usual).
//
// --Avery

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>

typedef unsigned long ulong;

int main(void) {
	FILE *f;
	ulong build=0,build_t;
	char s[25];
	time_t tm;
	struct tm *ptm;
	//////////////

	if (f=fopen("version.bin","rb")) {
		if (1==fread(&build_t,sizeof build_t,1,f))
			build=build_t;
	}

	++build;
//	printf("Incrementing to build %d\n",build);

	time(&tm);
	
	//memcpy(version_time, asctime(localtime(&tm)), sizeof(version_time)-1);
	ptm = localtime(&tm);
	strftime(s, sizeof(s), "%Y%m%d", ptm);
	s[sizeof(s)-1]=0;
	

	if (f=fopen("verstub.cpp","w")) {
		fprintf(f,
			"unsigned long version_num = %ld;\n"
			"extern const char *const version_time = \"%s\";\n"
			,build
			,s);
		fclose(f);
	}

	if (f=fopen("version.bin","wb")) {
		fwrite(&build,sizeof build,1,f);
		fclose(f);
	}

	return 0;
}
