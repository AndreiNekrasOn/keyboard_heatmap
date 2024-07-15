#include <ctime>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unordered_map>
#include <termios.h>

#define key_code __u16

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
    os << "\n" << kf.name << ": {\n\ttimes pressed: " << kf.numPresses <<
        "\n\ttime hold: " << ((double) kf.totalPressTime) / 1e6 <<
        " seconds\n},\n";
    return os;
}

void hideInput() {
    termios oldt;
    tcgetattr(1, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(1, TCSANOW, &newt);
}

int openDevice(libevdev **dev, const char *path) {
    int fd;
    int rc = 1;
    fd = open(path, O_RDONLY|O_NONBLOCK);
    rc = libevdev_new_from_fd(fd, dev);
    if (rc < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        exit(1);
    }
    return rc;
}

std::unordered_map<key_code, KeyFrequency *> keyloggerLoop(libevdev *dev,
        string stopWord) {
    std::unordered_map<key_code, KeyFrequency *> keyFrequency;
    int rc = 0;
    while (rc == 1 || rc == 0 || rc == -EAGAIN) {
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

    }
    return keyFrequency;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./keylogger.out <path/to/device> KEY_<STOP>");
        exit(1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    cout << "Start...\n";
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
