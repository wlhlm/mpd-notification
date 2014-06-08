/* See LICENSE file for licensing details. */

/* mpd connection options */
static const char *host = NULL;
static long port = 0;
static char *password = NULL;
static unsigned int timeout = 0;

/* Standard notification header. All tokens within it get replaced by song tags.
 * Will be overwritten by the -H commandline option. */
static const char *head = "Now Playing";
/* Standard notification body. All tokens within it get replaced by song tags.
 * Will be overwritten by the -B commandline option. */
static const char *body = "%a - %t";

/* Replacement for unknown tags. */
static const char *strunknown = "(unknown)";

/* Notification urgency */
static NotifyUrgency urgency = NOTIFY_URGENCY_LOW;

