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

#pragma once

#include <array>
#include <cuchar>
#include <fcntl.h>
#include <signal.h>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace termui::internal
{
/// Encapsulate a file handle to ensure proper closing
struct ScopedFd
{
    /** Open a file.
     * @param[in] path  path to file to open
     * @param[in] flags system call open flags
     * @return ScopedFd instance
     */
    static ScopedFd open(const std::string &path, int flags)
    {
        return ScopedFd{::open(path.c_str(), flags)};
    }

    ScopedFd()
        : fd{-1} {}
    ScopedFd(int _fd)
        : fd{_fd} {}
    ~ScopedFd()
    {
        if (isValid())
            ::close(fd);
    }

    // not copyable
    ScopedFd(const ScopedFd &) = delete;
    ScopedFd &operator=(const ScopedFd &) = delete;

    // movable
    ScopedFd(ScopedFd &&other) noexcept
        : fd{std::exchange(other.fd, -1)}
    {
    }
    ScopedFd &operator=(ScopedFd &&other) noexcept
    {
        std::swap(fd, other.fd);
        return *this;
    }

    /** Whether the contained file descriptor is valid.
     * @return whether the contained file descriptor is valid
     */
    bool isValid() const
    {
        return fd >= 0;
    }

    int fd; ///< file handle
};

/// Encapsulate signal catching
struct ScopedSignalCatcher
{
    ScopedSignalCatcher();
    ~ScopedSignalCatcher();

    // not copyable
    ScopedSignalCatcher(const ScopedSignalCatcher &) = delete;
    ScopedSignalCatcher &operator=(const ScopedSignalCatcher &) = delete;

    // not movable
    ScopedSignalCatcher(ScopedSignalCatcher &&) noexcept = delete;
    ScopedSignalCatcher &operator=(ScopedSignalCatcher &&) noexcept = delete;

    /** Signal handler
     * @param[in] signum  signal number
     */
    void handle(int signum)
    {
        ::write(fdEmit.fd, &signum, sizeof(signum));
    }

    // pipe ends
    ScopedFd fdEmit;                 ///< emit signum from signal handler towards the main loop
    ScopedFd fdReceive;              ///< get signum inside the main loop
    struct sigaction oldSigActWinch; ///< restore sigaction for SIGWINCH
    struct sigaction oldSigActInt;   ///< restore sigaction for SIGINT
    struct sigaction oldSigActTerm;  ///< restore sigaction for SIGTERM
};

/// Encapsulate tty handle
struct ScopedTty : public ScopedFd
{
    ScopedTty();
    ~ScopedTty();

    /// Retrieve the current screen size, update width and height fields
    void retrieveSize();

    struct termios origTermios; ///< original settings of the terminal, to restore
    int width;                  ///< screen width
    int height;                 ///< screen height
};

/// Add buffering and conversion to Tty
struct ScopedBufferedTty : public ScopedTty
{
    ScopedBufferedTty();

    /// Read data from tty, trying to fill rxBuffer
    void rxFd();

    /** Consume bytes from rxBuffer.
     * @param[in] n  number of bytes to consume from rxBuffer
     */
    void rxConsume(size_t n);

    /** Retrieve a char32_t character from rxBuffer.
     * @return -1 in case of unicode parsing error
     */
    char32_t rxC32();

    /** Append data in buffer for transmission.
     * @param[in] cmd  command to be sent to terminal
     */
    void txAppend(const char *cmd);
    /** Append data in buffer for transmission.
     * @param[in] str  command to be sent to terminal
     */
    void txAppend(const std::string &str);
    /** Append data in buffer for transmission.
     * @param[in] glyph  single unicode codepoint to transmit
     */
    void txAppend(char32_t glyph);

    /** Append a number for transmission, in ASCII.
     * @param[in] num  number to transmit; 42 will send "42"
     */
    void txAppendNumber(uint32_t num);

    /// Flush the output buffer to tty
    void txFlush();

    std::array<char, 8> m_rxBuffer; ///< buffer to receive commands from tty
    size_t m_rxFilled;              ///< number of bytes available in rxBuffer
    std::mbstate_t m_rxMbState;     ///< unicode decoder state on Rx stream
    std::mbstate_t m_txMbState;     ///< unicode decoder state on Rx stream
    std::vector<char> m_txBuffer;   ///< buffer to send commands to tty

private:
    /** Append unicode glyph in buffer for transmission.
     * @param[in] glyph  single unicode codepoint to transmit
     */
    void txAppendUnicodeGlyph(char32_t glyph);
};

inline void ScopedTty::retrieveSize()
{
    struct winsize screenSize;

    ioctl(fd, TIOCGWINSZ, &screenSize);

    width = screenSize.ws_col;
    height = screenSize.ws_row;
}

inline void ScopedBufferedTty::rxConsume(size_t n)
{
    if (n < m_rxFilled)
    {
        std::memmove(&m_rxBuffer[0], &m_rxBuffer[n], m_rxFilled - n);
        m_rxFilled -= n;
    }
    else
        m_rxFilled = 0;
}

inline char32_t ScopedBufferedTty::rxC32()
{
    char32_t result = 0;
    if (m_rxFilled == 0)
        return result;

    const size_t rc = std::mbrtoc32(&result, m_rxBuffer.data(), m_rxFilled, &m_rxMbState);
    if (rc == (size_t)-1 or rc == 0)
    {
        /* invalid UTF-8 stream
         * => consume 1 byte to progress and synchronize again
         */
        rxConsume(1);
    }
    else if (rc == (size_t)-2)
    {
        /* incomplete UTF-8 multibyte sequence
         * => do nothing, will retry when more data
         */
    }
    else
    {
        /// char properly decoded
        rxConsume(rc);
    }
    return result;
}

inline void ScopedBufferedTty::txAppend(const char *cmd)
{
    while (*cmd != '\0')
        m_txBuffer.emplace_back(*cmd++);
}
inline void ScopedBufferedTty::txAppend(const std::string &str)
{
    txAppend(str.c_str());
}
inline void ScopedBufferedTty::txAppend(char32_t glyph)
{
    if (glyph < 128)
    {
        // ASCII value, no conversion needed
        m_txBuffer.emplace_back(static_cast<char>(glyph));
    }
    else
    {
        txAppendUnicodeGlyph(glyph);
    }
}
inline void ScopedBufferedTty::txAppendNumber(uint32_t num)
{
    txAppend(std::to_string(num));
}
} // namespace termui::internal