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
 * Create a UI in a terminal.
 */

#include <cmath>
#include <sys/epoll.h>

#include "termui.h"

#include "termui_input_esc_seq.inl"

namespace termui
{

namespace commands
{
    /// enter alternate screen mode
    constexpr const char *smcup = "\e[?1049h\e[22;0;0t";
    /// exit alternate screen mode
    constexpr const char *rmcup = "\e[?1049l\e[23;0;0t";
    /// clear screen
    constexpr const char *clear = "\e[H\e[2J";
    /// enter keypad mode
    constexpr const char *smkx = "\e[?1h\e=";
    /// exit keypad mode
    constexpr const char *rmkx = "\e[?1l\e>";
    /// make cursor invisible
    constexpr const char *civis = "\e[?25l";
    /// restore cursor to normal mode
    constexpr const char *cnorm = "\e[?12l\e[?25h";
} // namespace commands

namespace epollFd
{
    enum
    {
        Signal, ///< slot for signal catcher
        Tty,    ///< slot for tty reading
    };
}

std::u32string toU32String(const std::string &str)
{
    std::u32string result{};
    // allocate enough space
    result.reserve(str.size());

    // get unicode char 1 by 1
    const char *input = str.c_str();
    size_t remainingInput = str.size();
    std::mbstate_t mbstate{};
    while (remainingInput > 0)
    {
        char32_t glyph;
        const size_t rc = std::mbrtoc32(&glyph, input, remainingInput, &mbstate);
        if (rc == 0 or rc > MB_CUR_MAX)
        {
            // invalid UTF-8 stream
            throw TermUiException{"invalid UTF-8 steam"};
        }

        input += rc;
        remainingInput -= rc;
        result += glyph;
    }

    return result;
}

/** Reduce length of a string
 * @note This functions assumes that strU32.size() > wantedSize.
 * @param[in,out] strU32      string to reduce
 * @param[in]     wantedSize  size of strU32 at exit
 * @param[in]     clipStart   whether clipping shall be done at start ("…ong text") or end ("too long t…")
 */
static void clipString(std::u32string &strU32, size_t wantedSize, bool clipStart = false)
{
    if (wantedSize == 0)
        strU32.clear();
    else if (clipStart)
    {
        // clip string at the beginning and use ellipsis as first char
        strU32 = U"…" + strU32.substr(strU32.size() - wantedSize + 1);
    }
    else
    {
        // clip string at the end and use ellipsis as last char
        strU32.resize(wantedSize - 1);
        strU32 += U'…';
    }
}

Color Color::fromHsv(float hue, float saturation, float value)
{
    // https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB
    const float chroma = saturation * value;
    const float minimum = value - chroma;
    const uint8_t colFull = std::round(255 * value);
    const uint8_t colLow = std::round(255 * minimum);
    const uint8_t colInter = std::round(255 * (minimum + chroma * (1 - std::abs(std::fmod(hue / 60, 2) - 1))));
    if (hue <= 60)
        return Color::fromRgb(colFull, colInter, colLow);
    else if (hue <= 120)
        return Color::fromRgb(colInter, colFull, colLow);
    else if (hue <= 180)
        return Color::fromRgb(colLow, colFull, colInter);
    else if (hue <= 240)
        return Color::fromRgb(colLow, colInter, colFull);
    else if (hue <= 300)
        return Color::fromRgb(colInter, colLow, colFull);
    else
        return Color::fromRgb(colFull, colLow, colInter);
}

TermUi::TermUi()
    : m_sigCatcher{}, m_tty{}, m_epoll{::epoll_create1(0)},
      m_frameBuffer{}, m_dirty{false},
      m_colorFg{Color::fromPalette(7)},
      m_colorBg{Color::fromPalette(0)}
{
    if (not m_tty.isValid())
        throw TermUiException{"cannot open /dev/tty"};

    // setup the terminal
    m_tty.txAppend(commands::smcup);
    m_tty.txAppend(commands::smkx);
    m_tty.txAppend(commands::civis);
    m_tty.txAppend(commands::clear);
    reset();
    publish();

    // register fds monitored by epoll
    struct epoll_event event
    {
    };
    event.events = EPOLLIN;
    event.data.u32 = epollFd::Signal;
    if (::epoll_ctl(m_epoll.fd, EPOLL_CTL_ADD, m_sigCatcher.fdReceive.fd, &event) < 0)
        throw TermUiExceptionErrno{"epoll_ctl, register signal catcher pipe"};
    event.data.u32 = epollFd::Tty;
    if (::epoll_ctl(m_epoll.fd, EPOLL_CTL_ADD, m_tty.fd, &event) < 0)
        throw TermUiExceptionErrno{"epoll_ctl, register tty"};
}

TermUi::~TermUi()
{
    // restore terminal
    m_tty.txAppend(commands::clear);
    m_tty.txAppend(commands::cnorm);
    m_tty.txAppend(commands::rmkx);
    m_tty.txAppend(commands::rmcup);
    m_tty.txFlush();
}

void TermUi::reset()
{
    // get updated screen size
    m_tty.retrieveSize();

    // resize frameBuffer
    m_frameBuffer.resize(m_tty.width * m_tty.height);

    // reset frameBuffer
    for (auto &cell : m_frameBuffer)
    {
        cell.reset(m_colorFg, m_colorBg);
    }

    m_dirty = true;
}

void TermUi::publish()
{
    if (m_dirty)
    {
        // first clear the screen
        m_tty.txAppend(commands::clear);

        // draw each cell
        Effect currentEffect{};
        Color currentFg{}; // invalid
        Color currentBg{}; // invalid
        int x = 0;
        int y = 0;
        for (const auto &cell : m_frameBuffer)
        {
            // handle formatting
            updateGraphicSettings(currentEffect, currentFg, currentBg,
                                  cell.effect, cell.colorFg, cell.colorBg);

            // draw glyph
            m_tty.txAppend(cell.glyph);

            // on line change, replace the cursor to avoid shift accumulation on screen resize
            x++;
            if (x >= m_tty.width)
            {
                y++;
                // place cursor at the beginning of next line
                m_tty.txAppend("\e[" + std::to_string(y + 1) + "H");
                x = 0;
            }
        }
        // reset color and formatting
        m_tty.txAppend("\e[0m");

        // flush to screen
        m_tty.txFlush();
        m_dirty = false;
    }
}

void TermUi::addStdU32String(int y, int x, const std::u32string &strU32, Color colorFg, Color colorBg, Effect effect)
{
    // add each glyph to its cell
    for (char32_t glyph : strU32)
    {
        addGlyph(y, x, glyph, colorFg, colorBg, effect);
        x++;
    }
}

void TermUi::addStringN(int y, int x, const std::string &str, int width, TextAlignment alignment, Color colorFg, Color colorBg, Effect effect)
{
    // convert the UTF8 string to glyphs
    std::u32string strU32 = toU32String(str);

    // manage alignment
    if (strU32.size() == (size_t)width)
    {
        // exactly fits, nothing to do
    }
    else if (strU32.size() > (size_t)width)
    {
        // string too long, need to clip
        clipString(strU32, width, alignment.isClipStart());
    }
    else
    {
        // string too short, need to align
        switch (alignment.getMode())
        {
        case TextAlignment::kLeft:
            strU32.resize(width, U' '); // expand with spaces
            break;
        case TextAlignment::kRight:
            strU32.insert(0, width - strU32.size(), U' '); // insert needed spaces
            break;
        case TextAlignment::kCentered:
            // insert needed spaces on left side
            strU32.insert(0, (width - strU32.size()) / 2, U' ');
            strU32.resize(width, U' '); // expand with spaces
            break;
        }
    }

    // process the string
    addStdU32String(y, x, strU32, colorFg, colorBg, effect);
}

void TermUi::addStringsN(int y, int x,
                         const std::string &strLeft, const std::string &strMiddle, const std::string &strRight,
                         int width, Color colorFg, Color colorBg, Effect effect)
{
    // convert the UTF8 strings to glyphs
    std::u32string strU32Left{}, strU32Middle{}, strU32Right{};
    if (not strLeft.empty())
        strU32Left = toU32String(strLeft);
    if (not strMiddle.empty())
        strU32Middle = toU32String(strMiddle);
    if (not strRight.empty())
        strU32Right = toU32String(strRight);

    // avoid overlap of fields
    int endLeft = strU32Left.size();
    int startMiddle = strU32Middle.empty()
                          ? width
                          : width / 2 - (strU32Middle.size() + 1) / 2;
    if (endLeft >= startMiddle - 1)
    {
        endLeft = std::min(endLeft, width / 3 - 1);
        startMiddle = std::max(startMiddle, width / 3 + 1);
    }
    int endMiddle = strU32Middle.empty()
                        ? 0
                        : startMiddle + strU32Middle.size();
    int startRight = width - strU32Right.size();
    if (endMiddle >= startRight - 1)
    {
        endMiddle = std::min(endMiddle, 2 * width / 3 - 1);
        startRight = std::max(startRight, 2 * width / 3 + 1);
    }
    if (endLeft >= startRight - 1)
    {
        endLeft = std::min(endLeft, width / 2 - 1);
        startRight = std::max(startRight, width / 2 + 1);
    }

    // clip strings if needed
    if (strU32Left.size() > (size_t)endLeft)
        clipString(strU32Left, endLeft);
    if (strU32Middle.size() > (size_t)(endMiddle - startMiddle))
        clipString(strU32Middle, endMiddle - startMiddle);
    if (strU32Right.size() > (size_t)(width - startRight))
        clipString(strU32Right, width - startRight);

    // build the string
    std::u32string strU32{};
    strU32.reserve(width);
    strU32 = strU32Left;
    if (not strU32Middle.empty())
    {
        strU32.resize(startMiddle, ' ');
        strU32 += strU32Middle;
    }
    strU32.resize(startRight, ' ');
    strU32 += strU32Right;

    // process the string
    addStdU32String(y, x, strU32, colorFg, colorBg, effect);
}

void TermUi::addFString(int y, int x, const std::u32string &formattedStr, int width)
{
    // start with default settings
    Color colorFg = m_colorFg;
    Color colorBg = m_colorBg;
    Effect effect = 0;

    auto *cellPtr = &m_frameBuffer[y * m_tty.width + x];

    for (char32_t c : formattedStr)
    {
        if (width == 0)
            break;
        if (U32Format::isU32Format(c))
        {
            if (U32Format::isEffect(c))
                effect = U32Format::getEffect(c);
            else if (U32Format::isColorFg(c))
                colorFg = U32Format::getColor(c);
            else if (U32Format::isColorBg(c))
                colorBg = U32Format::getColor(c);
        }
        else
        {
            cellPtr->colorFg = colorFg;
            cellPtr->colorBg = colorBg;
            cellPtr->effect = effect;
            cellPtr->glyph = c;
            cellPtr++;
            width--;
        }
    }
    // add spaces until width
    while (width > 0)
    {
        cellPtr->colorFg = colorFg;
        cellPtr->colorBg = colorBg;
        cellPtr->effect = effect;
        cellPtr->glyph = U' ';
        cellPtr++;
        width--;
    }
    m_dirty = true;
}

Event TermUi::waitForEvent(int timeoutMs)
{
    publish();

    // check if tty has already something available
    {
        Event event = getEventTty();
        if (event.isValid())
            return event;
    }

    struct epoll_event event;
    int nbEvents = ::epoll_wait(m_epoll.fd, &event, 1, timeoutMs);
    if (nbEvents < 0)
    {
        if (errno != EINTR and errno != EAGAIN)
            throw TermUiExceptionErrno{"epoll_wait"};
        // wait interrupted by a signal => consider there is no event
        nbEvents = 0;
    }

    if (nbEvents == 0)
        return {};

    // read the associated fd to get the event
    switch (event.data.u32)
    {
    case epollFd::Signal:
        return getEventSigCatcher();
    case epollFd::Tty:
        return getEventTty();
    default:
        throw TermUiException{"unsupported epoll_wait result"};
    }
}

Event TermUi::getEventSigCatcher()
{
    int signum;
    const ssize_t nbRead = ::read(m_sigCatcher.fdReceive.fd, &signum, sizeof(signum));
    if (nbRead < 0)
    {
        if (errno != EINTR and errno != EAGAIN)
            throw TermUiExceptionErrno{"signal catcher pipe read"};
        return {};
    }
    if (nbRead != sizeof(signum))
        throw TermUiException{"signal catcher pipe inconsistent read error"};

    return Event::fromSignal(signum);
}

Event TermUi::getEventTty()
{
    // complete the rxBuffer to decode complex commands
    m_tty.rxFd();
    // get a unicode char
    char32_t c = m_tty.rxC32();
    if (c == 0) // invalid
        return {};

    if (c <= 26)
        // Ctrl + Letter are encoded as value 1 to 26
        return Event::fromCtrl(c - 1);

    if (c != 27)
        // any kind of printable character, maybe in unicode
        return Event{c};

    /* c == 27: for non printable keys, terminal uses an escape encoding
     * identify it
     */
    {
        size_t consumed;
        const char32_t event = identify_esc_seq(m_tty.m_rxBuffer.data(), m_tty.m_rxFilled, consumed);
        if (event > 0)
        {
            // sequence found
            m_tty.rxConsume(consumed);
            return Event{event};
        }
    }

    // not matching a known sequence: then it is just a "Esc"
    return Event{c};
}

void TermUi::updateColorSetting(Color color, bool isFg)
{
    if (color.isPalette())
    {
        if (color.paletteIndex() < 8)
        {
            m_tty.txAppendNumber((isFg ? 30 : 40) + color.paletteIndex());
        }
        else
        {
            m_tty.txAppend(isFg ? "38;5;" : "48;5;");
            m_tty.txAppendNumber(color.paletteIndex());
        }
    }
    else
    {
        // RGB
        m_tty.txAppend(isFg ? "38;2;" : "48;2;");
        m_tty.txAppendNumber(color.red());
        m_tty.txAppend(';');
        m_tty.txAppendNumber(color.green());
        m_tty.txAppend(';');
        m_tty.txAppendNumber(color.blue());
    }
}

void TermUi::updateGraphicSettings(
    Effect &currentEffect,
    Color &currentFg,
    Color &currentBg,
    Effect wantedEffect,
    Color wantedFg,
    Color wantedBg)
{
    if (wantedEffect != currentEffect or wantedFg != currentFg or wantedBg != currentBg)
    {
        bool needSeparator = false;
        bool forceColorSet = false; // force color on reset

        // at least some change is needed, start the graphic command
        m_tty.txAppend("\e[");

        if (wantedEffect != currentEffect)
        {
            // reset all
            m_tty.txAppend('0');
            forceColorSet = true;
            // set the wanted effect(s)
            for (int bit = Effect::kFirstBit; bit <= Effect::kLastBit; bit++)
            {
                if (wantedEffect.value() & (1 << bit))
                {
                    // enable effect
                    m_tty.txAppend(';');
                    m_tty.txAppendNumber(bit);
                }
            }
            currentEffect = wantedEffect;
            needSeparator = true;
        }

        if (forceColorSet or wantedFg != currentFg)
        {
            if (needSeparator)
                m_tty.txAppend(';');

            updateColorSetting(wantedFg, true);
            currentFg = wantedFg;
            needSeparator = true;
        }

        if (forceColorSet or wantedBg != currentBg)
        {
            if (needSeparator)
                m_tty.txAppend(';');

            updateColorSetting(wantedBg, false);
            currentBg = wantedBg;
            needSeparator = true;
        }

        // end of the graphic command
        m_tty.txAppend('m');
    }
}

} // namespace termui