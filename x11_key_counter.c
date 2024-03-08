#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define ESCAPE_KEYCODE 36

typedef struct KeyInfo {
    int keycode;
    int pressTime;
    int totalTime;
    int counter;
    const char *label;
} KeyInfo;

void init_display(Display **dpy, int *opcode);
void init_select_event(Display **dpy);
KeyInfo *set_keyinfo(Display *dpy);
void print_keyinfo(const KeyInfo *keyInfo, int keycode);
void print_all_keyinfo(const KeyInfo *keyInfo);
void event_loop(Display *dpy, int opcode, KeyInfo *keyInfo);
void update_keyinfo(KeyInfo *keyInfo, int keycode, int isPress);
void write_keyinfo(KeyInfo *keyInfo, char *filename);


int main() {
    Display *dpy;
    int opcode;
    init_display(&dpy, &opcode);
    KeyInfo *keyInfo = set_keyinfo(dpy);
    init_select_event(&dpy);
    event_loop(dpy, opcode, keyInfo);
    return 0;
}

void init_display(Display **dpy, int *opcode) {
    *dpy = XOpenDisplay(NULL);
    int event, error;
    if (!XQueryExtension(*dpy, "XInputExtension", opcode, &event, &error)) {
        printf("X Input extension not available.\n");
        exit(1);
    }
    int major = 2, minor = 0;
    if (XIQueryVersion(*dpy, &major, &minor) == BadRequest) {
        printf("XI2 not available. Server supports %d.%d\n", major, minor);
        exit(1);
    }
}

KeyInfo *set_keyinfo(Display *dpy) {
    KeyInfo *keyInfo = malloc(sizeof(KeyInfo) * 256);
    for (int i = 0; i < 256; i++) {
        keyInfo[i].keycode = i;
        keyInfo[i].totalTime = 0;
        keyInfo[i].pressTime = 0;
        keyInfo[i].counter = 0;
        keyInfo[i].label = XKeysymToString(XkbKeycodeToKeysym(dpy, i, 0, 0));
    }
    return keyInfo;
}

void init_select_event(Display **dpy) {
    XIEventMask eventmask;
    unsigned char mask[(XI_LASTEVENT + 7) / 8];
    memset(mask, 0, sizeof(mask));
    eventmask.deviceid = XIAllDevices;
    eventmask.mask_len = sizeof(mask);
    eventmask.mask = mask;
    // TODO: 
    // XISetMask(mask, XI_ButtonPress);
    // XISetMask(mask, XI_Motion);
    XISetMask(mask, XI_KeyPress);
    XISetMask(mask, XI_KeyRelease);
    XISelectEvents(*dpy, DefaultRootWindow(*dpy), &eventmask, 1);
}



void print_keyinfo(const KeyInfo *keyInfo, int keycode) {
    const char *formatMessage =
        "Keycode: %d\tLabel: %s\tTotalTime: %d\tCounter: %d\n";
    printf(formatMessage, keycode, keyInfo[keycode].label,
                        keyInfo[keycode].totalTime, keyInfo[keycode].counter);
}

void print_all_keyinfo(const KeyInfo *keyInfo) {
    printf("{\n");
    for (int i = 0; i < 256; i++) {
        if (keyInfo[i].counter > 0) {
            print_keyinfo(keyInfo, i);
        }
    }
    printf("}\n");
}

void update_keyinfo(KeyInfo *keyInfo, int keycode, int isPress) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int updateTime = tv.tv_sec * 1000 + tv.tv_usec / 1000; // ms
    if (isPress) {
        keyInfo[keycode].pressTime = updateTime;
        keyInfo[keycode].counter++;
    } else if (keyInfo[keycode].counter > 0) {
        keyInfo[keycode].totalTime += updateTime - keyInfo[keycode].pressTime;
        keyInfo[keycode].pressTime = 0;
    }
}

void event_loop(Display *dpy, int opcode, KeyInfo *keyInfo) {
    XEvent ev;
    for (;;) {
        XGenericEventCookie *cookie = &ev.xcookie;
        XNextEvent(dpy, &ev);
        if (cookie->type != GenericEvent || cookie->extension != opcode) {
            continue;
        }
        if (XGetEventData(dpy, cookie)) {
            switch (cookie->evtype) {
            case XI_KeyPress:
                {
                    XIDeviceEvent *keyEvent = (XIDeviceEvent *)cookie->data;
                    update_keyinfo(keyInfo, keyEvent->detail, 1); 
                    break;
                }
            case XI_KeyRelease:
                {
                    XIDeviceEvent *keyEvent = (XIDeviceEvent *)cookie->data;
                    update_keyinfo(keyInfo, keyEvent->detail, 0); 
                    if (keyEvent->detail == ESCAPE_KEYCODE) {
                        print_all_keyinfo(keyInfo);
                    }
                    break;
                }
            }
            XFreeEventData(dpy, &ev.xcookie);
        }
    }
}

void write_keyinfo(KeyInfo *keyInfo, char *filename) {
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "Keycode\tLabel\tTotalTime\tCounter\n");
    for (int i = 0; i < 256; i++) {
        if (keyInfo[i].counter > 0) {
            fprintf(fp, "%d\t%s\t%d\t%d\n",
                        keyInfo[i].keycode, keyInfo[i].label,
                        keyInfo[i].totalTime, keyInfo[i].counter);
        }
    }
    fclose(fp);
}

