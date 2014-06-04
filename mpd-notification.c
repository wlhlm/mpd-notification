/* See LICENSE file for licensing details. */

/* Includes */
#include <mpd/client.h>
#include <libnotify/notify.h>
#include <glib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

/* Macro definitions */
/* Get array length on compile time */
#define LENGTH(a) (sizeof(a) / sizeof(a[0]))

/* Type declarations */
/* Format string in which tokens get replaced by song tags. */
typedef char Format;

/* Configuration */
#include "config.h"

/* Global variable definitions */
static struct mpd_connection *connection = NULL;
static bool verbose = false;
static NotifyNotification *notification = NULL;
static char *notification_body = NULL, *notification_body_escaped = NULL;
static char *notification_head = NULL, *notification_head_escaped = NULL;
static struct mpd_song *song = NULL;
static enum mpd_state state;

/* Function declarations */
static const char *get_mpd_song_tag(const struct mpd_song *song, enum mpd_tag_type type);
static enum mpd_state get_mpd_state(struct mpd_connection *connection);
static const char *get_tag_token_replacement(const char token, const struct mpd_song *song);
static NotifyNotification *init_notify(const char *name);
static size_t get_replaced_format_string_length(const Format *format, const struct mpd_song *song);
static char *replace_tag_tokens_all(const Format *format, const struct mpd_song *song);
static const bool show_notify_notification(NotifyNotification *notification);
void die(const char *errstr, ...);
static void usage(const char *name);

/* Implementation */
/* Retrives a tag from a mpd song object */
static const char *
get_mpd_song_tag(const struct mpd_song *song, enum mpd_tag_type type)
{
	const char *tag;

	tag = mpd_song_get_tag(song, type, 0);
	if (!tag) {
		tag = strunknown;
	}

	return tag;
}

/* Retrives the current status of mpd */
static enum mpd_state
get_mpd_state(struct mpd_connection *connection)
{
	struct mpd_status *status;
	enum mpd_state state;

	status = mpd_run_status(connection);
	if (status == NULL) {
		return -1;
	}

	state = mpd_status_get_state(status);
	mpd_status_free(status);

	return state;
}

/* Replace all tokens in the format string. */
static const char *
get_tag_token_replacement(const char token, const struct mpd_song *song)
{
	unsigned int i;
	struct mpd_tag_token {
		char token;
		enum mpd_tag_type tag;
	};
	const struct mpd_tag_token table[] = {
		{ 'a', MPD_TAG_ARTIST },
		{ 'b', MPD_TAG_ALBUM },
		{ 'A', MPD_TAG_ALBUM_ARTIST },
		{ 't', MPD_TAG_TITLE },
		{ 'T', MPD_TAG_TRACK },
		{ 'n', MPD_TAG_NAME },
		{ 'g', MPD_TAG_GENRE },
		{ 'd', MPD_TAG_DATE },
		{ 'c', MPD_TAG_COMPOSER },
		{ 'p', MPD_TAG_PERFORMER },
		{ 'C', MPD_TAG_COMMENT },
		{ 'D', MPD_TAG_DISC }
	};

	for (i = 0; i < LENGTH(table); i++) {
		if (table[i].token == token) {
			return get_mpd_song_tag(song, table[i].tag);
		}
	}

	return NULL;
}

/* Initialize libnotify; return a fresh notification object */
static NotifyNotification *
init_notify(const char *name)
{
	NotifyNotification *notification;

	if (!notify_init(name)) {
		return NULL;
	}

	notification = notify_notification_new(name, NULL, "sound");
	notify_notification_set_urgency(notification, urgency);

	return notification;
}

static size_t
get_replaced_format_string_length(const Format *format, const struct mpd_song *song)
{
	size_t size = strlen(format);

	while ((format = strchr(format, '%')) != NULL) {
		if (format[1] == '\0') break;
		else if (format[1] == '%') size--;
		else {
			const char *replacement;
			replacement = get_tag_token_replacement(format[1], song);
			if (replacement == NULL) continue;
			/* Subtract the token itself. */
			size += strlen(replacement) - 2;
		}
		format += 2;
	}

	return size;
}

/* Replaces all tokens in a given Format string with the tags of the currently
 * playing song. */
static char *
replace_tag_tokens_all(const Format *format, const struct mpd_song *song)
{
	char *format_position = NULL;
	char *new, *new_end;
	size_t offset;
	size_t size = get_replaced_format_string_length(format, song);

	new = calloc(size+1, sizeof(char));
	if (new == NULL) return NULL;
	new_end = new;

	while ((format_position = strchr(format, '%')) != NULL) {
		offset = format_position - format;
		strncpy(new_end, format, offset);
		new_end += offset;

		if (format_position[1] == '\0') break;
		else if (format_position[1] == '%') {
			new_end[0] = '%';
			new_end++;
		}
		else {
			const char *replacement;

			replacement = get_tag_token_replacement(format_position[1], song);
			if (replacement != NULL) {
				size_t replacement_size = strlen(replacement);

				strncpy(new_end, replacement, replacement_size);
				new_end += replacement_size;
			}
		}

		/* Skip the token. */
		format = format_position + 2;
	}

	/* Copy the rest. */
	strcpy(new_end, format);

	new[size] = '\0';
	return new;
}

/* Send the notification to the notification daemon. */
static const bool
show_notify_notification(NotifyNotification *notification)
{
	GError *error = NULL;

	return notify_notification_show(notification, &error);
}

/* Print error message and exit. */
void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

/* Print program usage string. */
static void
usage(const char *name)
{
	die("Usage: %s [-V] [-h HOST] [-p PORT] [-P PASSWORD] [-v] [-H HEADING] [-B BODY]\n", name);
}

/* Main procedure */
int
main(int argc, char *argv[])
{
	int opt;

	/* Parse commandline options */
	while ((opt = getopt(argc, argv, "Vh:p:P:vH:B:")) != -1) {
		switch (opt) {
		case 'V':
			printf("%s, version %s\n", NAME, VERSION);
			exit(EXIT_SUCCESS);
			break;
		case 'h':
			host = optarg;
			break;
		case 'p':
			port = strtol(optarg, NULL, 10);
			if ((errno == ERANGE && (port == LONG_MAX || port == LONG_MIN))
					|| (errno != 0 && port == 0)
					|| (port < 0 || port > 65535)) {
				die("Invalid port number. Exiting.\n");
			}
			break;
		case 'P':
			password = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		case 'B':
			body = optarg;
			break;
		case 'H':
			head = optarg;
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	connection = mpd_connection_new(host, port, timeout);
	if (connection == NULL) {
		die("%s: Cannot connect to mpd - Out of memory. Exiting.\n", argv[0]);
	}
	if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
		fprintf(stderr, "%s: %s\n", argv[0], mpd_connection_get_error_message(connection));
		mpd_connection_free(connection);
		exit(EXIT_FAILURE);
	}

	if (password != NULL) {
		if (!mpd_run_password(connection, password)) {
			die("%s: Wrong password. Exiting.\n", argv[0]);
		}
	}

	notification = init_notify(NAME);

	/* Main loop */
	while (mpd_run_idle_mask(connection, MPD_IDLE_PLAYER)) {
		if ((state = get_mpd_state(connection)) == -1) {
			die("%s: Cannot get mpd status. Exiting.\n", argv[0]);
		}

		if (state == MPD_STATE_PLAY) {
			song = mpd_run_current_song(connection);
			if (song == NULL) {
				continue;
			}

			notification_head = replace_tag_tokens_all(head, song);
			if (verbose) {
				printf("Head: %s\n", notification_head);
			}
			notification_head_escaped = g_markup_escape_text(notification_head, -1);
			free(notification_head);

			if (body != NULL) {
				notification_body = replace_tag_tokens_all(body, song);
				if (verbose) {
					printf("Body: %s\n", notification_body);
				}
				notification_body_escaped = g_markup_escape_text(notification_body, -1);
				free(notification_body);
			}

			mpd_song_free(song);

			notify_notification_update(notification, notification_head_escaped, notification_body_escaped, NULL);
			if (!show_notify_notification(notification)) {
				fprintf(stderr, "%s: Cannot display notification.\n", argv[0]);
			}

			g_free(notification_head_escaped);
			g_free(notification_body_escaped);
		}
	}

	mpd_connection_free(connection);
	g_object_unref(G_OBJECT(notification));
	notify_uninit();
}
