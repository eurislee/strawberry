#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include "strawberry.h"
#include "desktop.h"
#include "settings.h"
#include "subscribe.h"
#include "tree.h"

subscriber_list_t *make_subscriber(FILE *stream, char *fifo_path, int field, int count)
{
	subscriber_list_t *sb = calloc(1, sizeof(subscriber_list_t));
	sb->prev = sb->next = NULL;
	sb->stream = stream;
	sb->fifo_path = fifo_path;
	sb->field = field;
	sb->count = count;
	return sb;
}

void remove_subscriber(subscriber_list_t *sb)
{
	if (sb == NULL) {
		return;
	}
	subscriber_list_t *a = sb->prev;
	subscriber_list_t *b = sb->next;
	if (a != NULL) {
		a->next = b;
	}
	if (b != NULL) {
		b->prev = a;
	}
	if (sb == subscribe_head) {
		subscribe_head = b;
	}
	if (sb == subscribe_tail) {
		subscribe_tail = a;
	}
	if (restart) {
		int cli_fd = fileno(sb->stream);
		fcntl(cli_fd, F_SETFD, ~FD_CLOEXEC & fcntl(cli_fd, F_GETFD));
	} else {
		fclose(sb->stream);
		unlink(sb->fifo_path);
	}
	free(sb->fifo_path);
	free(sb);
}

void add_subscriber(subscriber_list_t *sb)
{
	if (subscribe_head == NULL) {
		subscribe_head = subscribe_tail = sb;
	} else {
		subscribe_tail->next = sb;
		sb->prev = subscribe_tail;
		subscribe_tail = sb;
	}
	int cli_fd = fileno(sb->stream);
	fcntl(cli_fd, F_SETFD, FD_CLOEXEC | fcntl(cli_fd, F_GETFD));
	if (sb->field & SBSC_MASK_REPORT) {
		print_report(sb->stream);
		if (sb->count-- == 1) {
			remove_subscriber(sb);
		}
	}
}

int print_report(FILE *stream)
{
	fprintf(stream, "%s", status_prefix);
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		fprintf(stream, "%c%s", (mon == m ? 'M' : 'm'), m->name);
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			char c = (is_urgent(d) ? 'u' : (d->root == NULL ? 'f' : 'o'));
			if (m->desk == d) {
				c = toupper(c);
			}
			fprintf(stream, ":%c%s", c, d->name);
		}
		if (m->desk != NULL) {
			fprintf(stream, ":L%c", LAYOUT_CHR(m->desk->layout));
			if (m->desk->focus != NULL) {
				node_t *n = m->desk->focus;
				if (n->client != NULL) {
					fprintf(stream, ":T%c", STATE_CHR(n->client->state));
				} else {
					fprintf(stream, ":T@");
				}
				int i = 0;
				char flags[5];
				if (n->sticky) {
					flags[i++] = 'S';
				}
				if (n->private) {
					flags[i++] = 'P';
				}
				if (n->locked) {
					flags[i++] = 'L';
				}
				if (n->marked) {
					flags[i++] = 'M';
				}
				flags[i] = '\0';
				fprintf(stream, ":G%s", flags);
			}
		}
		if (m != mon_tail) {
			fprintf(stream, "%s", ":");
		}
	}
	fprintf(stream, "%s", "\n");
	return fflush(stream);
}

void put_status(subscriber_mask_t mask, ...)
{
	subscriber_list_t *sb = subscribe_head;
	int ret;
	while (sb != NULL) {
		subscriber_list_t *next = sb->next;
		if (sb->field & mask) {
			if (sb->count > 0) {
				sb->count--;
			}
			if (mask == SBSC_MASK_REPORT) {
				ret = print_report(sb->stream);
			} else {
				char *fmt;
				va_list args;
				va_start(args, mask);
				fmt = va_arg(args, char *);
				vfprintf(sb->stream, fmt, args);
				va_end(args);
				ret = fflush(sb->stream);
			}
			if (ret != 0 || sb->count == 0) {
				remove_subscriber(sb);
			}
		}
		sb = next;
	}
}

void prune_dead_subscribers(void)
{
	subscriber_list_t *sb = subscribe_head;
	while (sb != NULL) {
		subscriber_list_t *next = sb->next;
		// Is the subscriber's stream closed?
		if (write(fileno(sb->stream), NULL, 0) == -1) {
			remove_subscriber(sb);
		}
		sb = next;
	}
}
