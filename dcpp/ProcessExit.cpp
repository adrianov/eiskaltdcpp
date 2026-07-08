/*
 * Records clean vs abrupt exits and provides async-signal-safe fatal notes.
 */

#include "stdinc.h"
#include "ProcessExit.h"

#include "Util.h"
#include "File.h"

#ifndef _WIN32
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace dcpp {

namespace {

string statePath() {
    return Util::getPath(Util::PATH_USER_LOCAL) + "Logs/run.state";
}

#ifndef _WIN32
char fatalStatePath[512] = {};

void cacheFatalStatePath() {
    if(fatalStatePath[0])
        return;
    const string path = statePath();
    if(path.size() >= sizeof(fatalStatePath))
        return;
    memcpy(fatalStatePath, path.c_str(), path.size() + 1);
}

void writeState(const char* text) {
    try {
        const string path = statePath();
        File::ensureDirectory(path);
        File f(path, File::WRITE, File::TRUNCATE | File::CREATE);
        f.write(text);
        f.write("\n");
    } catch(const FileException&) {
    }
}

void writeStateFd(const char* text, size_t len) {
    cacheFatalStatePath();
    if(!fatalStatePath[0])
        return;
    const int fd = ::open(fatalStatePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd < 0)
        return;
    ::write(fd, text, len);
    ::write(fd, "\n", 1);
    ::close(fd);
}
#endif

const char* signalName(int sig) {
    switch(sig) {
    case 1: return "SIGHUP";
    case 2: return "SIGINT";
    case 6: return "SIGABRT";
    case 7: return "SIGBUS";
    case 9: return "SIGKILL";
    case 11: return "SIGSEGV";
    case 13: return "SIGPIPE";
    case 15: return "SIGTERM";
    default: return nullptr;
    }
}

} // namespace

#ifndef _WIN32
void installSigpipeIgnore() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &sa, nullptr) == -1)
        return;

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);
}

void blockSigpipeInThread() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);
}

void noteFatalSignal(int sig) noexcept {
    char buf[32];
    const int n = snprintf(buf, sizeof(buf), "signal:%d", sig);
    if(n <= 0)
        return;
    writeStateFd(buf, static_cast<size_t>(n));
}
#endif

void markSessionRunning() {
#ifndef _WIN32
    cacheFatalStatePath();
#endif
    writeState("running");
}

void markSessionNormal() noexcept {
    writeState("normal");
}

string checkPreviousSession() {
    try {
        const string path = statePath();
        if(!Util::fileExists(path))
            return string();
        File f(path, File::READ, File::OPEN);
        string line = f.read();
        while(!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if(line.empty() || line == "normal")
            return string();
        if(line == "running")
            return _("Previous session ended abruptly (no clean shutdown was recorded)");
        if(line.compare(0, 7, "signal:") == 0) {
            const int sig = Util::toInt(line.substr(7));
            if(const char* name = signalName(sig))
                return str(F_("Previous session ended with signal %1% (%2%)") % sig % name);
            return str(F_("Previous session ended with signal %1%") % sig);
        }
        return str(F_("Previous session ended with state: %1%") % line);
    } catch(const FileException&) {
        return string();
    }
}

} // namespace dcpp
