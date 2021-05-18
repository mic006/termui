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
 */

#include <grp.h>
#include <pwd.h>

#include "csys.h"

namespace csys
{

ScopedDir ScopedFd::opendir(bool keepOriginalFd)
{
    int dirFd = fd;
    if (keepOriginalFd)
    {
        // duplicate the file handle
        dirFd = ::fcntl(fd, F_DUPFD_CLOEXEC, 0);
        if (dirFd < 0)
            throw CsysExceptionErrno{"fcntl(F_DUPFD_CLOEXEC)", path};
    }

    DIR *dir = ::fdopendir(dirFd);
    if (dir == nullptr)
    {
        if (keepOriginalFd)
            ::close(dirFd); // close the useless duplicate
        throw CsysExceptionErrno{"fdopendir", path};
    }

    if (not keepOriginalFd)
        fd = -1; // current ScopedFd does not own the fd anymore
    return ScopedDir{path, dir};
}

std::string ScopedFd::readlinkat(const std::string &relPath, size_t size) const
{
    if (size == 0)
        size = PATH_MAX;
    else
        size++; // add room for a final \0
    char buff[size];
    ssize_t res = ::readlinkat(fd, relPath.c_str(), buff, size);
    if (res < 0)
    {
        const std::string logPath = "#" + std::to_string(fd) + ":" + relPath;
        throw CsysExceptionErrno{"readlinkat", logPath};
    }
    // add the terminating \0, not done by readlinkat
    if ((size_t)res == size)
        res--;
    buff[res] = '\0';
    return std::string(buff);
}

const std::string &UidGidNameReader::getUidName(uint16_t uid)
{
    // access / create element
    std::string &name = m_uidNames[uid];
    if (not name.empty())
        return name; // name already known

    // need to retrieve the name
    const struct passwd *passwdPtr = ::getpwuid(uid);
    name = passwdPtr == nullptr ? std::to_string(uid) : passwdPtr->pw_name;
    return name;
}

const std::string &UidGidNameReader::getGidName(uint16_t gid)
{
    // access / create element
    std::string &name = m_gidNames[gid];
    if (not name.empty())
        return name; // name already known

    // need to retrieve the name
    const struct group *groupPtr = ::getgrgid(gid);
    name = groupPtr == nullptr ? std::to_string(gid) : groupPtr->gr_name;
    return name;
}

ScopedFd Signal::signalFd() const
{
    if (::sigprocmask(SIG_BLOCK, &m_mask, NULL) < 0)
        throw CsysExceptionErrno{"sigprocmask", path};
    int fd = ::signalfd(-1, &m_mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (fd < 0)
        throw CsysExceptionErrno{"signalfd", path};
    return ScopedFd{path, fd};
}

ScopedFd Poll::create()
{
    const int fd = ::epoll_create1(EPOLL_CLOEXEC);
    if (fd < 0)
        throw CsysExceptionErrno{"epoll_create1", path};
    return ScopedFd{path, fd};
}

Poll::Poll()
    : m_epollFd{create()}
{
}

void Poll::add(ScopedFd &fd, uint32_t events, PollEventCallback &&handler)
{
    if (not bool(fd))
        throw CsysException{"poll: trying to add invalid fd"};
    const int sysFd = fd.fd;
    if (m_monitoredFds.contains(fd.fd))
        throw CsysException{"poll: conflict when adding fd #" + std::to_string(fd.fd)};

    m_monitoredFds.try_emplace(sysFd, fd, std::move(handler));
    struct epoll_event ev
    {
        .events = events,
        .data = {.fd = sysFd},
    };
    if (::epoll_ctl(m_epollFd.fd, EPOLL_CTL_ADD, sysFd, &ev) < 0)
    {
        // remove added element
        (void)m_monitoredFds.erase(sysFd);
        throw CsysExceptionErrno{"epoll_ctl(EPOLL_CTL_ADD)", path};
    }
}

void Poll::remove(ScopedFd &fd)
{
    if (not bool(fd))
        throw CsysException{"poll: trying to remove invalid fd"};
    const int sysFd = fd.fd;
    (void)m_monitoredFds.erase(sysFd);
    if (::epoll_ctl(m_epollFd.fd, EPOLL_CTL_DEL, sysFd, nullptr) < 0)
        throw CsysExceptionErrno{"epoll_ctl(EPOLL_CTL_DEL)", path};
}

void Poll::waitAndProcessEvents(int timeoutMs, int nbSimulatenousEvents)
{
    struct epoll_event events[nbSimulatenousEvents];

    // wait for events or timeout
    int nbEvents = ::epoll_wait(m_epollFd.fd, events, nbSimulatenousEvents, timeoutMs);
    if (nbEvents < 0 and errno != EINTR)
        throw CsysExceptionErrno{"epoll_wait", path};

    // process received events
    for (int i = 0; i < nbEvents; i++)
    {
        PollFdContext &ctx = m_monitoredFds.at(events[i].data.fd);
        if (ctx.handler)
        {
            ctx.handler(*this, ctx.fd, events[i].events);
        }
    }
}

MainPollHandler::MainPollHandler()
    : m_exitRequested{false},
      m_exitStatus{EXIT_SUCCESS},
      m_unlockEventfd{ScopedFd::eventfd()}
{
    add(m_unlockEventfd, EPOLLIN, [this](Poll &, ScopedFd &, uint32_t events) {
        this->unlockHandler(events);
    });
}

void MainPollHandler::setSignals(Signal &&sigObj)
{
    m_signalFd = sigObj.signalFd();
    add(m_signalFd, EPOLLIN, [this](Poll &, ScopedFd &, uint32_t events) {
        this->signalHandlerDispatch(events);
    });
}

int MainPollHandler::runForever()
{
    while (not m_exitRequested.load())
    {
        waitAndProcessEvents();
    }
    return m_exitStatus;
}

void MainPollHandler::signalHandlerDispatch(uint32_t events)
{
    if ((events & EPOLLERR) != 0)
        abort();

    // consume the data
    struct signalfd_siginfo fdsi;
    size_t dataRead = m_signalFd.read(&fdsi, sizeof(fdsi));
    if (dataRead != sizeof(fdsi))
        abort();

    if (not m_exitRequested.load())
    {
        if (m_signalCbks.contains(fdsi.ssi_signo))
        {
            // call the handler
            m_signalCbks[fdsi.ssi_signo](fdsi.ssi_signo);
        }
        else
        {
            // exit with unhandled signal
            setRequestTermination(fdsi.ssi_signo);
        }
    }
}

void MainPollHandler::unlockHandler(uint32_t events)
{
    if ((events & EPOLLERR) != 0)
        abort();

    // event is only for wakeup, content is not relevant
    (void)m_unlockEventfd.eventfdRead();
}

} // namespace csys