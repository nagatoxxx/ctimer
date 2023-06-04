#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <getopt.h>
#include <libnotify/notify.h>

struct time {
    unsigned hours;
    unsigned minutes;
    unsigned seconds; };

enum {COUNTDOWN, TARGET}; // modes

static void seconds_to_time(struct time* t, long seconds);
static long time_to_seconds(const struct time* t);
static int decrease_time(struct time* t);
static void countdown(struct time* t);
static void target(struct time* t);
static void strrev(char *string);
static int parse_time_string(struct time* t, char* string, int mode);
static void sig_handler(int signal);
static void print_help(void);
static int check_time_string(const char *string);
static void standart_form_time_string(struct time* t);
static void background();
#ifdef NOTIFY
static void send_notification(const char* message);
#endif


int
main(int argc, char* argv[]) {
    signal(SIGINT, sig_handler);
    
    if (argc == 1) {
        print_help();
        return 0;
    }
    
    char time_string[32];
    int res = 0;
    int mode = 0;

    bool bg = false;
    
    char notification_message[64];
    bool notify = false;

    while ((res = getopt(argc, argv, "c:t:n:hb")) != -1) {
        switch (res) {
            case 'h':
                print_help();
                return 0;
            case 'c':
                mode = COUNTDOWN;
                strcpy(time_string, optarg);
                break;
            case 't':
                mode = TARGET;
                strcpy(time_string, optarg);
                break;
            case 'n':
                strcpy(notification_message, optarg);
                notify = true;
                break;
            case 'b':
                bg = true;
                break;
                
        }
    }

    struct time timer = {0, 0, 0};
    if (check_time_string(time_string))
        goto exit_error;

    parse_time_string(&timer, time_string, mode);
    if (mode == COUNTDOWN) 
        standart_form_time_string(&timer);
    
    if (mode == TARGET && (timer.hours >= 24 || timer.minutes >= 60 ||
                timer.seconds >= 60)) {
        goto exit_error;
    } 
    else if (mode == TARGET)
        target(&timer);

    if (bg)
        background();

    countdown(&timer);
    if (notify) {
        send_notification(notification_message);
    }

    return EXIT_SUCCESS;

exit_error:
    fprintf(stderr, "error: invalid time or duration\n");
    exit(EXIT_FAILURE);
}



static int
parse_time_string(struct time* t, char* string, int mode) {
    char *buf = NULL;
    int i = 0;
    unsigned* units[3];
    switch (mode) {
        case COUNTDOWN:
            strrev(string);
            buf = strtok(string, ":");

            units[0] = &t->seconds;
            units[1] = &t->minutes;
            units[2] = &t->hours;

            while (buf != NULL) {
                strrev(buf);
                *(units[i++]) = atoi(buf);
                buf = strtok(NULL, ":");
            }
            break;

        case TARGET:
            sscanf(string, "%u:%u", &t->hours, &t->minutes);
            break;
    }

    return 0;
}

static void
strrev(char *string) {
    int i = 0;
    int j = strlen(string) - 1;
    char c = '\0';
    while (i < j) {
        c = string[i];
        string[i] = string[j];
        string[j] = c;
        i++; j--;
    }
}

static void // calculate time to certain
target(struct time* t) {
    long seconds = time_to_seconds(t);
    long timer = 0;
    time_t current_time = time(NULL);
    struct tm* utc_time = localtime(&current_time);
    long utc_seconds = utc_time->tm_hour * 3600 + utc_time->tm_min * 60
        + utc_time->tm_sec;

    timer = seconds - utc_seconds;
    if (utc_seconds > seconds) {
        timer = 24 * 3600 - labs(timer);
    }

    seconds_to_time(t, timer);
}

static void
countdown(struct time* t) {
    printf("\x1B[?25l"); // hide cursor
    while (1) {
        printf("%02u:%02u:%02u\n", t->hours, t->minutes, t->seconds); 
        printf("\x1B[1A");
        printf("\x1B[K");
        if (!decrease_time(t))
            break;
        sleep(1);
    }
    printf("\x1B[?25h"); // show cursor
}

static void
seconds_to_time(struct time* t, long seconds) {
    t->hours = seconds / 3600;
    seconds -= 3600 * t->hours;
    t->minutes = seconds / 60;
    seconds -= 60 * t->minutes;
    t->seconds = seconds;
}

static long
time_to_seconds(const struct time* t) {
    return t->hours * 3600 + t->minutes * 60 + t->seconds;
}

static int // return 0 if time is over
decrease_time(struct time* t) {
    unsigned* seconds = &(t->seconds);
    unsigned* minutes = &(t->minutes);
    unsigned* hours = &(t->hours);

    if (*seconds == 0) {
        if (*minutes == 0) {
            if (*hours == 0) {
                return 0;
            }
            --(*hours);
            *minutes = 59;
            *seconds = 59;
            goto ret;
        }
        --(*minutes);
        *seconds = 59;
        goto ret;
    }
    --(*seconds);
ret:
    return 1;
}

static void sig_handler(int signal) {
    printf("\x1B[?25h"); // show cursor
    printf("\x1B[1A");
    printf("\x1B[K");
    exit(1);
}

static void print_help(void) {
    printf(
        "usage:\n"
        "    ctimer [option]...\n"
        "\noptions:\n"
        "    -h\t\t\tshow this message\n"
        "    -t <time string>\tset a timer until a certain time\n"
        "    -c <time string>\tset a timer for a given time\n"
        "    -n <message>\tsend notification when time is up\n"
        "    -b\t\t\tput the timer in the background\n"
        "\ntime string:\n"
        "    <h:m> for -t\n"
        "    <s> or <m:s> or <h:m:s> for -c\n"
        );
}

static int check_time_string(const char *string) {
    int c = 0; // count of ':'
    for (const char* p = string; *p != '\0'; p++) {
        if ((*p > '9' || *p < '0') && (*p != ':'))
            return 1;
        if (c > 2)
            return 1;
        if (*p == ':')
            c += 1;
    }

    return 0;
}

static void standart_form_time_string(struct time* t) {
    while (t->seconds >= 60) {
        t->seconds -= 60;
        t->minutes += 1;
    }
    while (t->minutes >= 60) {
        t->minutes -= 60;
        t->hours += 1;
    }
}

#ifdef NOTIFY
static void
send_notification(const char *message) {
    notify_init("ctimer");
    NotifyNotification *notify;
    notify = notify_notification_new("timer", message, NULL);

    /* notify_notification_set_urgency(notify, NOTIFY_URGENCY_CRITICAL); */

    notify_notification_show(notify, NULL);
    notify_uninit();
}
#endif

static void background() {
    pid_t pid;
    pid = fork();

    if (pid > 0) {
        //parent
        exit(EXIT_SUCCESS);
    }

    printf("%i\n", getpid());

    setsid();

    printf("\x1B[?25h"); // show cursor
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}
