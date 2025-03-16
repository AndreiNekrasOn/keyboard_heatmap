#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>

#define key_code unsigned long long

#ifdef __unix__         
#include <unistd.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <termios.h>
#elif defined(_WIN32) || defined(WIN32) 
#define OS_Windows
#include <windows.h>
#include <winuser.h>
// https://stackoverflow.com/a/26085827
#include <stdint.h> // portable: uint64_t   MSVC: __int64 
// MSVC defines this in winsock2.h!?
int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970 
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}

#define libevdev void
#endif

using std::cout;
using std::endl;
using std::ostream;
using std::string;

class KeyFrequency {
private:
    int numPresses;
    long lastPressTime;
    long totalPressTime;
    string name;

    timeval tv;

    long getTime() {
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1e6 + tv.tv_usec ; // microseconds
    }
public:
    KeyFrequency(string name) {
        this->name = name;
        lastPressTime = getTime();
        numPresses = 0;
        totalPressTime = 0;
    }

    void update(bool keyDown) {
        long updateTime = getTime();
        if (keyDown) {
            totalPressTime += updateTime - lastPressTime;
        } else {
            numPresses++;
        }
        lastPressTime = updateTime;
    }

    time_t getTotalPressTime() {
        return totalPressTime;
    }

    int getNumPresses() {
        return numPresses;
    }

    friend std::ostream &operator<<(ostream &os, const KeyFrequency &kf);
};

ostream &operator<<(ostream &os, const KeyFrequency &kf) {
    os << kf.name << "," << kf.numPresses << "," <<
        ((double) kf.totalPressTime) / 1e6 << "\n";
    return os;
}

void hideInput() {
    #ifdef  __unix__
    termios oldt;
    tcgetattr(1, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(1, TCSANOW, &newt);
    #endif
}

int openDevice(libevdev **dev, const char *path) {
    #ifdef __unix__
    int fd;
    int rc = 1;
    fd = open(path, O_RDONLY|O_NONBLOCK);
    rc = libevdev_new_from_fd(fd, dev);
    if (rc < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        exit(1);
    }
    return rc;
    #endif
    return 0;
}

// play sound on linux using system() call
int play_sound(string music_filename) {
    #ifdef __unix__
    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    } else if (pid == 0) { // child
        string cmd = "mpv " + music_filename + " > /dev/null";
        int result = system(cmd.c_str());
        if (result != 0) {
            std::cerr << "Error: Failed to play sound '" << music_filename << "'" << endl;
            _exit(-1); // Return error code
        }
        _exit(0);
    }
    return pid;
    #endif
    return 0;
}


std::unordered_map<key_code, KeyFrequency *> keyloggerLoop(libevdev *dev,
        string stopWord) {
    std::unordered_map<key_code, KeyFrequency *> keyFrequency;
    int rc = 0;
    int i = 0;
    while (rc == 1 || rc == 0 || rc == -EAGAIN) {
        #ifdef __unix__
        input_event ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc != 0 || ev.type != EV_KEY) {
            continue;
        }
        if (stopWord.compare(
                libevdev_event_code_get_name(EV_KEY, ev.code)) == 0) {
            break;
        }
        if (keyFrequency.find(ev.code) == keyFrequency.end()) {
            keyFrequency[ev.code] = new KeyFrequency(
                libevdev_event_code_get_name(EV_KEY, ev.code));
        } else {
            keyFrequency[ev.code]->update(ev.value != 0);
        }
        if (ev.value != 0) {
            play_sound("quack.opus");
        }
        #else
        if (GetAsyncKeyState(VK_ESCAPE)) {
            break; // Exit the loop if ESC is pressed
        }
        if (i > 100) {
            break;
        }
        for (Key)
            if (GetAsyncKeyState(key)) { // If the key is pressed
                keyFrequency[key - 'A'] = new KeyFrequency("" + key); // wtf
            }
        }
        Sleep(100);
        #endif
    }
    return keyFrequency;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./keylogger.out <path/to/device> KEY_<STOP>");
        exit(1);
    }
    hideInput();
    libevdev *dev = NULL;
    int rc = openDevice(&dev, argv[1]);
    auto keyFrequency = keyloggerLoop(dev, argv[2]);
    for (auto it : keyFrequency) {
        cout << *it.second;
    }
    cout << endl;
    return 0;
}
