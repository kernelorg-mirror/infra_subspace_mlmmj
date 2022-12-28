#include <string.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "getaddrsfromfile.h"
#include "chomp.h"

int
getaddrsfromfile(struct strlist *slist, FILE *f, int max)
{
	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;

	if (max == 0)
		return -1;
	if (f == NULL)
		return -1;
	if (feof(f))
		return (-1);
	while ((linelen = getline(&line, &linecap, f)) > 0) {
		chomp(line);
		if (*line == '\0')
			continue;
		slist->count++;
		slist->strs = (char **)xrealloc(slist->strs,
			sizeof(char *) * slist->count);
		slist->strs[slist->count -1] = xstrdup(line);
		if (slist->count >= max)
			break;
	}
	if (feof(f))
		return (0);
	return (1);
}
