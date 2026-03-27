#include <string.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "getaddrsfromfile.h"
#include "chomp.h"

int
getaddrsfromfile(strlist *slist, FILE *f, size_t max)
{
	char *line = NULL;
	size_t linecap = 0;

	if (max == 0)
		return -1;
	if (f == NULL)
		return -1;
	if (feof(f))
		return (-1);
	while (getline(&line, &linecap, f) > 0) {
		chomp(line);
		if (*line == '\0')
			continue;
		vec_push(slist, xstrdup(line));
		if (slist->len >= max)
			break;
	}
	if (feof(f))
		return (0);
	return (1);
}
