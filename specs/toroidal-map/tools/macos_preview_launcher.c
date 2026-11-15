/* Local preview launcher: keep concurrent VCMI clients' logs separate. */
#include <mach-o/dyld.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	char raw[PATH_MAX], executable[PATH_MAX];
	uint32_t size = sizeof(raw);
	if(_NSGetExecutablePath(raw, &size) != 0 || !realpath(raw, executable))
	{
		fprintf(stderr, "Cannot resolve preview launcher path\n");
		return 1;
	}
	char *name = strrchr(executable, '/');
	if(!name || strlcpy(name + 1, "vcmiclient", sizeof(executable) - (size_t)(name + 1 - executable))
		>= sizeof(executable) - (size_t)(name + 1 - executable))
		return 1;

	bool explicitLog = false;
	for(int i = 1; i < argc; ++i)
		if(strcmp(argv[i], "--logLocation") == 0 || strncmp(argv[i], "--logLocation=", 14) == 0)
			explicitLog = true;

	char temporary[] = "/tmp/vcmi-toroid-preview.XXXXXX";
	if(!explicitLog && !mkdtemp(temporary))
	{
		perror("Cannot create preview log directory");
		return 1;
	}
	char **arguments = calloc((size_t)argc + 3, sizeof(*arguments));
	if(!arguments)
		return 1;
	int next = 0;
	arguments[next++] = executable;
	if(!explicitLog)
	{
		arguments[next++] = "--logLocation";
		arguments[next++] = temporary;
	}
	for(int i = 1; i < argc; ++i)
		arguments[next++] = argv[i];
	execv(executable, arguments);
	perror("Cannot start VCMI client");
	free(arguments);
	return 1;
}
