/*
Copyright 2021 Michel Palleau

This file is part of sync-dir.

sync-dir is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

sync-dir is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with sync-dir. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file
 *
 * Wrap C system calls in C++ classes.
 * 
 * For all system calls, the returned value is checked for error
 * and an exception is raised in case of error.
 */

#pragma once

#include <atomic>
#include <csignal>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

namespace csys
{

/// Base exception for csys related errors
struct CsysException : public std::runtime_error
{
    CsysException(const std::string &message)
        : std::runtime_error{message} {}
};

/// Csys exception related to system calls, with display of errno
struct CsysExceptionErrno : public CsysException
{
    CsysExceptionErrno(const std::string &message, const std::string &path)
        : CsysException{message + " error on '" + path + "': " + std::strerror(errno)} {}
};

/** Dir handle encapsulation.
 *
 * Access to dir content via ::opendir, ::readdir and ::closedir.
 * 
 * Ensure proper resource freeing and check errors
 * on standard APIs.
 */
class ScopedDir
{
public:
    ScopedDir()
        : path{}, dir{} {}
    ScopedDir(const std::string &_path, DIR *_dir)
        : path{_path}, dir{_dir} {}
    ~ScopedDir()
    {
        if (dir != nullptr)
            ::closedir(dir);
    }

    // not copyable
    ScopedDir(const ScopedDir &) = delete;
    ScopedDir &operator=(const ScopedDir &) = delete;

    // movable
    ScopedDir(ScopedDir &&other) noexcept
        : path{std::exchange(other.path, "")}, dir{std::exchange(other.dir, nullptr)}
    {
    }
    ScopedDir &operator=(ScopedDir &&other) noexcept
    {
        std::swap(path, other.path);
        std::swap(dir, other.dir);
        return *this;
    }

    /// Whether dir handle is valid
    explicit operator bool() const
    {
        return dir != nullptr;
    }

    /// Read next entry from the directory
    const struct dirent *readdir()
    {
        return ::readdir(dir);
    }

protected:
    std::string path; ///< path, for logging
    DIR *dir;         ///< dir handle
};

/** File descriptor encapsulation.
 * 
 * System calls using file descriptors: ::open, ::read, ::write...
 *
 * Ensure proper resource freeing and check errors
 * on standard APIs.
 */
class ScopedFd
{
public:
    /// Open a file
    static ScopedFd open(const std::string &path, int flags)
    {
        const int fd = ::open(path.c_str(), flags | O_CLOEXEC);
        if (fd < 0)
            throw CsysExceptionErrno{"open", path};
        return ScopedFd{path, fd};
    }

    /// Create an eventfd
    static ScopedFd eventfd()
    {
        static constexpr auto path = "#eventfd";
        const int fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (fd < 0)
            throw CsysExceptionErrno{"eventfd", path};
        return ScopedFd{path, fd};
    }

    ScopedFd()
        : path{}, fd{-1} {}
    ScopedFd(const std::string &_path, int _fd)
        : path{_path}, fd{_fd} {}
    ~ScopedFd()
    {
        if (fd >= 0)
            ::close(fd);
    }

    // not copyable
    ScopedFd(const ScopedFd &) = delete;
    ScopedFd &operator=(const ScopedFd &) = delete;

    // movable
    ScopedFd(ScopedFd &&other) noexcept
        : path{std::exchange(other.path, "")}, fd{std::exchange(other.fd, -1)}
    {
    }
    ScopedFd &operator=(ScopedFd &&other) noexcept
    {
        std::swap(path, other.path);
        std::swap(fd, other.fd);
        return *this;
    }

    /// Whether file handle is valid
    explicit operator bool() const
    {
        return fd >= 0;
    }

    /// Open a new file from the current fd via ::openat
    ScopedFd openat(const std::string &relPath, int flags) const
    {
        const int newFd = ::openat(fd, relPath.c_str(), flags | O_CLOEXEC);
        const std::string logPath = "#" + std::to_string(fd) + ":" + relPath;
        if (newFd < 0)
            throw CsysExceptionErrno{"openat", logPath};
        return ScopedFd{logPath, newFd};
    }

    /** Use ::opendir to access directory content of the file descriptor.
     * 
     * Note: fdopendir() is taking the ownership of the given fd.
     * In particular, closedir() will close the fd.
     * 
     * @param[in] keepOriginalFd  whether the original ScopedFd is still valid on return
     *                            true -> fd is duplicated first to have an independent fd in ScopedDir
     *                            false -> fd ownership is transferred to the returned ScopedDir
     */
    ScopedDir opendir(bool keepOriginalFd);

    /** Read the symbolic link target from the current fd via ::readlinkat
     * 
     * @param[in] relPath  symlink path relative to current fd
     * @param[in] size     known length of the symlink (from stat)
     * 
     * @return symlink target
     */
    std::string readlinkat(const std::string &relPath, size_t size = 0) const;

    /** Get stat of a file relative from the current fd via ::statx
     * 
     * @param[in] relPath    target path relative to current fd
     * @param[in] mask       statx mask indicating the wanted fields
     * @param[out] statxbuf  statx output
     */
    void statx(const std::string &relPath, unsigned int mask, struct statx &statxbuf) const
    {
        const int res = ::statx(fd, relPath.c_str(),
                                AT_NO_AUTOMOUNT | AT_SYMLINK_NOFOLLOW | AT_STATX_DONT_SYNC,
                                mask,
                                &statxbuf);
        if (res < 0)
        {
            const std::string logPath = "#" + std::to_string(fd) + ":" + relPath;
            throw CsysExceptionErrno{"statx", logPath};
        }
    }

    /** Read data from fd
     * 
     * @param[out] buf target buffer, at least nbytes bytes
     * @param[in] nbytes  max number of bytes to read
     * 
     * @return number of bytes read (0 <= return <= nbytes)
     */
    size_t read(void *buf, size_t nbytes)
    {
        ssize_t bytesRead = ::read(fd, buf, nbytes);
        if (bytesRead < 0)
            throw CsysExceptionErrno{"read", path};
        return bytesRead;
    }

    /** Write data to fd
     * 
     * @param[in] buf source buffer to write
     * @param[in] nbytes  number of bytes to write
     * 
     * @return number of bytes written (0 <= return <= nbytes)
     */
    size_t write(void *buf, size_t nbytes)
    {
        ssize_t bytesWritten = ::write(fd, buf, nbytes);
        if (bytesWritten < 0)
            throw CsysExceptionErrno{"write", path};
        return bytesWritten;
    }

    /** Read data from fd, non blocking mode
     * 
     * @param[out] buf target buffer, at least nbytes bytes
     * @param[in] nbytes  max number of bytes to read
     * 
     * @return number of bytes read (0 <= return <= nbytes)
     */
    size_t readNonBlocking(void *buf, size_t nbytes)
    {
        ssize_t bytesRead = ::read(fd, buf, nbytes);
        if (bytesRead < 0)
        {
            if (errno != EINTR and errno != EAGAIN)
                throw CsysExceptionErrno{"read", path};
            bytesRead = 0;
        }
        return bytesRead;
    }

    /** Perform ioctl on fd
     * 
     * @param[in] request  ioctl request
     * @param[in,out] buf  buffer for request / result
     */
    void ioctl(unsigned long request, void *buf)
    {
        const int res = ::ioctl(fd, request, buf);
        if (res < 0)
            throw CsysExceptionErrno{"ioctl(" + std::to_string(request) + ")", path};
    }

    /** Advise system on fd usage.
     *
     * Note: no error check as advise is only an indication.
     */
    int posix_fadvise(off_t offset, off_t len,
                      int advise)
    {
        return ::posix_fadvise(fd, offset, len, advise);
    }

    /** Read data from eventfd
     * 
     * Data shall be available
     */
    eventfd_t eventfdRead()
    {
        eventfd_t value = 0;
        if (::eventfd_read(fd, &value) < 0)
            throw CsysExceptionErrno{"eventfd_read", path};
        return value;
    }

    /** Write data to eventfd
     * 
     * Data shall be available
     */
    void eventfdWrite(eventfd_t value)
    {
        if (::eventfd_write(fd, value) < 0)
            throw CsysExceptionErrno{"eventfd_write", path};
    }

protected:
    std::string path; ///< path, for logging
    int fd;           ///< file handle

    friend class Poll;
};

/** Convert uid or gid to name.
 * 
 * uid and gid are read only once (for each value)
 * from the system and are cached for future retrieval.
 */
class UidGidNameReader
{
public:
    UidGidNameReader() = default;
    ~UidGidNameReader() = default;

    // not copyable
    UidGidNameReader(const UidGidNameReader &) = delete;
    UidGidNameReader &operator=(const UidGidNameReader &) = delete;

    // not movable
    UidGidNameReader(UidGidNameReader &&) noexcept = delete;
    UidGidNameReader &operator=(UidGidNameReader &&) noexcept = delete;

    /// Get the name of the given uid
    const std::string &getUidName(uint16_t uid);

    /// Get the name of the given gid
    const std::string &getGidName(uint16_t gid);

private:
    std::map<uint16_t, std::string> m_uidNames; ///< cache mapping uid to name
    std::map<uint16_t, std::string> m_gidNames; ///< cache mapping gid to name
};

/// Manage signal handling
class Signal
{
public:
    Signal()
    {
        if (::sigemptyset(&m_mask) < 0)
            throw CsysExceptionErrno{"sigemptyset", path};
    }
    ~Signal() = default;

    // not copyable
    Signal(const Signal &) = delete;
    Signal &operator=(const Signal &) = delete;

    // not movable
    Signal(Signal &&) noexcept = delete;
    Signal &operator=(Signal &&) noexcept = delete;

    /// Add signal to mask
    void add(int sig)
    {
        if (::sigaddset(&m_mask, sig) < 0)
            throw CsysExceptionErrno{"sigaddset", path};
    }

    /** Create a signalfd capturing the configured signals
     * 
     * Note: the signals are blocked in current thread
     */
    ScopedFd signalFd() const;

protected:
    static constexpr auto path = "#signalfd"; ///< path used for sognalfd object
    sigset_t m_mask;                          ///< mask for signals to capture
};

// forward declaration
class Poll;
/** Callback on poll event
 * 
 * @param[in] poll    Poll instance
 * @param[in] fd     ScopedFd object on which the event occurred
 * @param[in] events bitmask of EPOLLxxx events (mostly EPOLLIN, EPOLLOUT, EPOLLERR)
 */
using PollEventCallback = std::function<void(Poll &poll, ScopedFd &fd, uint32_t events)>;

/// Encapsulate ::epoll
class Poll
{
protected:
    /// Fd managed by Poll
    struct PollFdContext
    {
        PollFdContext(ScopedFd &_fd, PollEventCallback &&_handler)
            : fd{_fd}, handler{std::move(_handler)}
        {
        }

        // not copyable
        PollFdContext(const PollFdContext &) = delete;
        PollFdContext &operator=(const PollFdContext &) = delete;

        // not movable
        PollFdContext(PollFdContext &&other) noexcept = delete;
        PollFdContext &operator=(PollFdContext &&other) noexcept = delete;

        ScopedFd &fd;
        PollEventCallback handler;
    };

public:
    Poll();
    ~Poll() = default;

    // not copyable
    Poll(const Poll &) = delete;
    Poll &operator=(const Poll &) = delete;

    // not movable
    Poll(Poll &&) noexcept = delete;
    Poll &operator=(Poll &&) noexcept = delete;

    /// add fd to poll
    void add(ScopedFd &fd, uint32_t events, PollEventCallback &&handler);

    /// remove fd from poll
    void remove(ScopedFd &fd);

    /// poll with given timeout in milliseconds and process retrieved events
    void waitAndProcessEvents(int timeoutMs = -1, int nbSimulatenousEvents = 8);

protected:
    /// create epoll instance
    static ScopedFd create();

    static constexpr auto path = "#poll";        ///< path used for epoll instance
    std::map<int, PollFdContext> m_monitoredFds; ///< fd monitored by Poll, indexed by fd
    ScopedFd m_epollFd;                          ///< epoll instance fd
};

/// Callback on signal reception
using SignalEventCallback = std::function<void(int sig)>;

/** Poll handler to use in main thread
 * 
 * Handler to use in main thread, which provides:
 * - epoll with dynamic addition / removal of fd
 * - signal handling
 * - asynchronous termination request
 */
class MainPollHandler : public Poll
{
public:
    MainPollHandler();

    /// Set signals handled by the MainPollHandler, call ONCE
    void setSignals(Signal &&sigObj);
    /// Set signals handled by the MainPollHandler, call ONCE
    template <typename... Args>
    void setSignals(Args &&...args)
    {
        setSignals(Signal(), args...);
    }
    /// Build the signal object from the list of signals
    template <typename... Args>
    void setSignals(Signal &&sigObj, int sig, Args &&...args)
    {
        sigObj.add(sig);
        setSignals(std::move(sigObj), args...);
    }

    void registerSignalHandler(int sig, SignalEventCallback &&handler)
    {
        m_signalCbks[sig] = std::move(handler);
    }

    /// Asynchronous termination request, wake main thread
    void requestTermination(int exitStatus = EXIT_SUCCESS)
    {
        setRequestTermination(exitStatus);
        // unlock main thread
        m_unlockEventfd.eventfdWrite(1);
    }

    /// Main loop, run forever until termination
    int runForever();

protected:
    /// Handle event from m_signalFd
    void signalHandlerDispatch(uint32_t events);

    /// Handle event from m_unlockEventfd
    void unlockHandler(uint32_t events);

    /// Set m_exitRequested and m_exitStatus if needed
    void setRequestTermination(int exitStatus)
    {
        // set m_exitRequested
        if (not m_exitRequested.exchange(true))
        {
            // first to set m_exitRequested, record exitStatus
            m_exitStatus = exitStatus;
        }
    }

    std::atomic<bool> m_exitRequested;               ///< whether exit of application is requested
    int m_exitStatus;                                ///< status to return on exit
    ScopedFd m_unlockEventfd;                        ///< eventfd fd to unlock main thread
    ScopedFd m_signalFd;                             ///< signalfd fd
    std::map<int, SignalEventCallback> m_signalCbks; ///< signal callbacks
};

} // namespace csys
