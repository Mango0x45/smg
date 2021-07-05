#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

#include "smg.h"

#define ARRAY_SIZE(x) ((sizeof(x)) / (sizeof(x[0])))

/* clang-format off */
/* Keep sorted for binary search, sort(1) is your best friend. */
static const struct Tag replace[] = {
	{ '"',  "\\(dq" },
	{ '\'', "\\(aq" },
	{ '-',  "\\-"   },
	{ '\\', "\\e"   },
	{ '^',  "\\(ha" },
	{ '`',  "\\(ga" },
	{ '~',  "\\(ti" }
};
/* clang-format on */

static const char *
one_line_strstr(const char *haystack, const char *needle)
{
	char c, sc;

	if ((c = *needle++)) {
		size_t len = strlen(needle);
		do
			do
				if (!(sc = *haystack++) || sc == '\n')
					return NULL;
			while (sc != c);
		while (strncmp(haystack, needle, len) != 0);
		haystack--;
	}
	return haystack;
}

/* https://hackr.io/blog/binary-search-in-c */
static void
parse_char(const char c)
{
	/* Static to avoid computing this every single time */
	static int array_size = ARRAY_SIZE(replace), array_middle = ARRAY_SIZE(replace) / 2;

	int first = 0, last = array_size;
	int middle = array_middle;

	while (first <= last) {
		if (replace[middle].symbol < c)
			first = middle + 1;
		else if (replace[middle].symbol > c)
			last = middle - 1;
		else {
			fputs(replace[middle].replace, stdout);
			return;
		}

		middle = (first + last) / 2;
	}

	putchar(c);
}

static noreturn void
die(const char *s)
{
	fprintf(stderr, PROGNAME ": %s", s);
	if (errno)
		fprintf(stderr, ": %s", strerror(errno));

	write(STDERR_FILENO, "\n", 1);
	exit(EXIT_FAILURE);
}

static noreturn void
usage(void)
{
	fputs("Usage: " PROGNAME " file\n", stderr);
	exit(EXIT_FAILURE);
}

ATTRIBUTE_MALLOC static char *
xstrndup(const char *s, size_t len)
{
	char *ret = strndup(s, len);
	if (!ret)
		die("strndup");
	return ret;
}

/* Hash function generated with gperf(1) using `make hash` */
static unsigned int
hash(register const char *str, register size_t len)
{
	static unsigned char asso_values[] = {
	        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	        13, 13, 13, 13, 13, 13, 13, 13, 5,  13, 13, 13, 13, 13, 13, 13, 13, 13, 0,  13,
	        13, 13, 13, 5,  0,  13, 0,  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	        5,  13, 13, 13, 13, 13, 13, 13, 13, 13, 0,  13, 13, 13, 13, 5,  0,  13, 0,  13,
	        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};
	return len + asso_values[(unsigned char) str[0]];
}

static const char *
set_metatag(char **v, const char *s)
{
	if (*v)
		die("Multiple definitions of the same metatag");

	for (; *s; s++)
		if (!(isblank(*s) || *s == '\n'))
			break;
	const char *start = s;

	/* This could happen if the input file consists of only metatags */
	s = strchr(s, '\n');
	if (!s)
		die("Missing newline");

	*v = xstrndup(start, s - start);
	return s;
}

static const char *
parse_sublist(const char *s, enum ListState *list_state)
{
	const char *start = s;
	for (++s; isblank(*s); s++) {}
	switch (*s) {
	case '-':
	case '+':
	case '*':
		for (++s; isblank(*s); s++) {}
		if (!*s)
			goto default_case;

		if (*list_state == ADDITIONAL_SUBLIST)
			puts(".IP");
		else
			*list_state = ADDITIONAL_SUBLIST;
		return --s;
	default:
default_case:
		*list_state = NOT_IN_LIST;
		return start;
		break;
	}
}

static const char *
parse_list(const char *s, enum ListState *list_state)
{
	const char *start = s;
	for (++s; isblank(*s); s++) {}
	if (!*s)
		return start;
	puts(".TP");
	*list_state = FIRST_SUBLIST;
	return --s;
}

static const char *
parse_emphasis(const char *s)
{
	const char op = *s++;
	const char *end = s, *token = (op == '*') ? "**" : "__";

	while ((end = one_line_strstr(end, token))) {
		if (*(end - 1) != '\\')
			break;
		end++;
	}

	if (!end) {
		putchar(op);
		return --s;
	}

	fputs((op == '*') ? "\\fB" : "\\fI", stdout);
	while (++s != end) {
		if (*s == '\\' && *(s + 1) != '\\')
			putchar(*++s);
		else
			parse_char(*s);
	}
	fputs("\\fR", stdout);

	return ++s;
}

static const char *
parse_codeblock(const char *s)
{
	unsigned int tags = 1;
	const char *start = s;

	while (*++s && *s != '\n') {
		if (*s != '`') {
			putchar(*start);
			return start;
		}
		tags++;
	}

	/* Setup the needle we're going to search for */
	char needle[tags + 1];
	memset(needle, '`', tags);
	needle[tags] = '\0';

	char *end = strstr(s, needle);
	char last = *(end + tags);
	if (!end || (last != '\n' && last != '\0') || *(end - 1) != '\n') {
		putchar(*start);
		return start;
	}

	puts(".EX");
	for (s++; s != end; s++)
		parse_char(*s);
	fputs(".EE", stdout);

	return s += tags - 1;
}

static const char *
parse_indent(const char *s)
{
	int indent = 4;
	const char op = *s;
	const char *start = s;

	while (*++s != '\n') {
		if (*s != op) {
			putchar(*start);
			return start;
		}
		indent += 4;
	}

	printf(".RS %d", (op == '>') ? indent : -indent);
	return --s;
}

static const char *
parse_header(const char *s, const char c)
{
	if (c == '=')
		goto header;
	else if (c == '-') {
		fputs(".SS ", stdout);
		goto out;
	}

	if (*++s == '#') {
		fputs(".SS ", stdout);
		s++;
	}
	else
header:
		fputs(".SH ", stdout);

out:
	/* Skip whitespace after the '#' */
	while (isblank(*s))
		s++;

	for (; *s && *s != '\n'; s++)
		putchar(*s);

	return --s;
}

static const char *
parse_paragraph(const char *s)
{
	while (*s == '\n')
		s++;

	puts(".PP");
	return --s;
}

static const char *
parse_metatag(const char *s, struct MetaData *md)
{
	const char *start = ++s;
	for (; *s; s++)
		if (isblank(*s) || *s == '\n')
			break;

	size_t len = s - start;
	if (s - start == 0)
		die("Blank metatag");

	unsigned int tag = hash(start, len);

	switch (tag) {
	case NAME:
		s = set_metatag(&md->name, s);
		break;
	case SECTION:
		s = set_metatag(&md->section, s);
		break;
	case DATE:
		s = set_metatag(&md->date, s);
		break;
	case VERSION:
		s = set_metatag(&md->version, s);
		break;
	case TITLE:
		s = set_metatag(&md->title, s);
		break;
	default:
		die("Invalid metatag");
	}

	return s;
}

static const char *
parse_metadata(const char *s)
{
	struct MetaData md = {0};

	for (unsigned int tags = 0; tags < 5; s++) {
		switch (*s) {
		case '@':
			s = parse_metatag(s, &md);
			tags++;
			break;
		case '\n':
			break;
		default:
			die("Couldn't find all metatags");
		}
	}

	printf(".TH \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"\n", md.name, md.section, md.date, md.version,
	       md.title);
	free(md.name);
	free(md.section);
	free(md.date);
	free(md.version);
	free(md.title);
	return s;
}

static void
parse_file(const char *s)
{
	bool newline = true;
	enum ListState list_state = NOT_IN_LIST;

	s = parse_metadata(s);
	for (; *s; s++) {
		if (*s == '\\') {
			switch (*++s) {
			case '\n':
				puts("\n.br");
				break;
			case '\\':
				parse_char('\\');
				break;
			case '|':
				fputs("\\|", stdout);
				break;
			default:
				putchar(*s);
			}
			newline = false;
		}
		else if (newline) {
			switch (*s) {
			case '.':
				fputs("\\&.", stdout);
				break;
			case '\n':
				s = parse_paragraph(s);
				continue;
			case '#':
				s = parse_header(s, 0);
				break;
			case '>':
			case '<':
				s = parse_indent(s);
				break;
			case '`':
				s = parse_codeblock(s);
				break;
			case '+':
			case '-':
			case '*':
				if (isblank(*(s + 1))) {
					s = parse_list(s, &list_state);
					break;
				}
				goto default_case;
			case '\t':
			case ' ':
				if (list_state != NOT_IN_LIST) {
					s = parse_sublist(s, &list_state);
					break;
				}
			default:
default_case:
				newline = false;
				if (list_state == FIRST_SUBLIST) {
					puts(".PP");
					list_state = NOT_IN_LIST;
				}

				/* Support underlining headings */
				const char *nl;
				if ((nl = strchr(s, '\n'))) {
					const char c = *++nl;
					if (c != '=' && c != '-')
						goto no_anchor_tag;

					while (*++nl == c) {}
					if (*nl != '\n')
						goto no_anchor_tag;
					(void) parse_header(s, c);
					s = --nl;
					break;
				}

				goto no_anchor_tag;
			}
			newline = false;
		}
		else {
no_anchor_tag:
			switch (*s) {
			case '\n':
				putchar('\n');
				newline = true;
				break;
			case '*':
			case '_':
				if (*(s + 1) == *s)
					s = parse_emphasis(s);
				else
					putchar(*s);
				break;
			case '.':
				if (*(s + 1) == '.' && *(s + 2) == '.') {
					fputs(".\\|.\\|.", stdout);
					s += 2;
				}
				else
					putchar('.');
				break;
			default:
				parse_char(*s);
			}
		}
	}
}

static const char *
load_file(char *file)
{
	FILE *fp = fopen(file, "r");
	if (!fp)
		die("fopen");

	struct stat sb;
	int ret = stat(file, &sb);
	if (ret == -1)
		die("stat");

	char *buf = calloc(sb.st_size + 1, sizeof(char));
	if (!buf)
		die("calloc");

	size_t read = fread(buf, sizeof(char), sb.st_size, fp);
	if (read != (size_t) sb.st_size)
		die("fread");

	(void) fclose(fp);
	return (const char *) buf;
}

int
main(int argc, char **argv)
{
	if (argc != 2)
		usage();
	setlocale(LC_ALL, "");

	const char *buf = load_file(argv[1]);
	parse_file(buf);

#ifdef DEBUG /* Make valgrind more useful */
	free((void *) buf);
#endif
	return EXIT_SUCCESS;
}
