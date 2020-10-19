/*
Copyright 2020 Michel Palleau

This file is part of termui.

termui is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

termui is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with termui. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file
 *
 * Internal definitions for TermUi.
 */

#include "termui.h"

namespace termui::internal
{
/// Available signal catcher instance
static ScopedSignalCatcher *signalCatcher;

/// Signal handler
static void sigHandler(int signum)
{
    if (signalCatcher != nullptr)
        signalCatcher->handle(signum);
}

ScopedSignalCatcher::ScopedSignalCatcher()
{
    // create pipe to forward signals to main loop
    int pipeFds[2];
    if (::pipe2(pipeFds, O_NONBLOCK))
        throw TermUiExceptionErrno{"pipe2"};
    fdReceive = ScopedFd{pipeFds[0]};
    fdEmit = ScopedFd{pipeFds[1]};

    // register signal handler
    signalCatcher = this;
    struct sigaction sigAction = {};
    sigAction.sa_handler = sigHandler;
    sigaction(SIGWINCH, &sigAction, &oldSigActWinch);
    sigaction(SIGINT, &sigAction, &oldSigActInt);
    sigaction(SIGTERM, &sigAction, &oldSigActTerm);
}

ScopedSignalCatcher::~ScopedSignalCatcher()
{
    // restore signal handlers
    sigaction(SIGWINCH, &oldSigActWinch, nullptr);
    sigaction(SIGINT, &oldSigActInt, nullptr);
    sigaction(SIGTERM, &oldSigActTerm, nullptr);
    signalCatcher = nullptr;
}

ScopedTty::ScopedTty()
    : ScopedFd{ScopedFd::open("/dev/tty", O_RDWR)}, width{-1}, height{-1}
{
    if (isValid())
    {
        ::tcgetattr(fd, &origTermios);

        struct termios modifiedTermios = origTermios;
        ::cfmakeraw(&modifiedTermios);
        // make terminal read non-blocking
        modifiedTermios.c_cc[VMIN] = 0;
        modifiedTermios.c_cc[VTIME] = 0;
        ::tcsetattr(fd, TCSAFLUSH, &modifiedTermios);

        retrieveSize();
    }
}

ScopedTty::~ScopedTty()
{
    if (isValid())
    {
        ::tcsetattr(fd, TCSAFLUSH, &origTermios);
    }
}

ScopedBufferedTty::ScopedBufferedTty()
    : m_rxBuffer{}, m_rxFilled{0}, m_rxMbState{}, m_txMbState{}, m_txBuffer{}
{
    m_txBuffer.reserve(4096);
}

void ScopedBufferedTty::rxFd()
{
    if (m_rxFilled < m_rxBuffer.size())
    {
        ssize_t nbRead = ::read(fd, &m_rxBuffer[m_rxFilled], m_rxBuffer.size() - m_rxFilled);
        if (nbRead < 0)
        {
            if (errno != EINTR and errno != EAGAIN)
                throw TermUiExceptionErrno{"tty read"};
            nbRead = 0;
        }
        m_rxFilled += nbRead;
    }
}

void ScopedBufferedTty::txAppendUnicodeGlyph(char32_t glyph)
{
    // perform UTF-8 encoding
    char mbs[MB_CUR_MAX + 1]; // extra slot for null termination
    // convert glyph to multi byte sequence
    const std::size_t size = std::c32rtomb(mbs, glyph, &m_txMbState);
    if (size > MB_CUR_MAX)
        throw TermUiException("c32rtomb: invalid unicode glyph " + std::to_string((uint32_t)glyph));
    mbs[size] = 0; // add null termination
    txAppend(mbs);
}

void ScopedBufferedTty::txFlush()
{
    size_t sent = 0;
    // send all data
    do
    {
        ssize_t nbWritten = ::write(fd, &m_txBuffer[sent], m_txBuffer.size() - sent);
        if (nbWritten < 0)
        {
            if (errno != EINTR and errno != EAGAIN)
                throw TermUiExceptionErrno{"tty write"};
            nbWritten = 0;
        }
        sent += nbWritten;
    } while (sent < m_txBuffer.size());
    // flush
    m_txBuffer.clear();
}
} // namespace termui::internal