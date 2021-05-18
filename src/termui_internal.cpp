/*
Copyright 2021 Michel Palleau

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
ScopedTty::ScopedTty()
    : ScopedFd{csys::ScopedFd::open("/dev/tty", O_RDWR)}, width{-1}, height{-1}
{
    if (bool(*this))
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
    if (bool(*this))
    {
        // restore terminal
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
        m_rxFilled += readNonBlocking(&m_rxBuffer[m_rxFilled], m_rxBuffer.size() - m_rxFilled);
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
        sent += write(&m_txBuffer[sent], m_txBuffer.size() - sent);
    } while (sent < m_txBuffer.size());
    // flush
    m_txBuffer.clear();
}
} // namespace termui::internal