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
 * Create a UI in a terminal.
 */

#pragma once

#include <cstring>
#include <stdexcept>
#include <vector>

#include "termui_internal.h"

namespace termui
{
/// Base class for Termui exceptions
struct TermUiException : public std::runtime_error
{
    TermUiException(const std::string &message)
        : std::runtime_error{message} {}
};

/// Termui exceptions related to system calls, with display of errno
struct TermUiExceptionErrno : public TermUiException
{
    TermUiExceptionErrno(const std::string &message)
        : TermUiException{message + " error : " + std::strerror(errno)} {}
};

/** Convert UTF-8 encoded string to u32string.
 * @param[in] str  input UTF-8 string
 * @return u32string with the same content
 */
std::u32string toU32String(const std::string &str);

/// Event retrieved by TermUi
class Event
{
public:
    static constexpr char32_t invalidMask = 0x80000000;
    static constexpr char32_t ctrlMask = 0x40000000;
    static constexpr char32_t altMask = 0x20000000;   // non-printable chars only
    static constexpr char32_t shiftMask = 0x10000000; // non-printable chars only
    static constexpr char32_t specialMask = 0x08000000;
    static constexpr char32_t valueMask = 0x01FFFFF; // unicode on 21 bits

    // constant for special events
    static constexpr char32_t kInvalid = invalidMask;

    static constexpr char32_t kCtrlC = ctrlMask | 'C';
    static constexpr char32_t kBackspace = 0x7f;
    static constexpr char32_t kTab = ctrlMask | 'I';
    static constexpr char32_t kEnter = ctrlMask | 'M';
    static constexpr char32_t kEscape = '\e';

    static constexpr char32_t kArrowUp = specialMask | 0x1;
    static constexpr char32_t kArrowDown = specialMask | 0x2;
    static constexpr char32_t kArrowRight = specialMask | 0x3;
    static constexpr char32_t kArrowLeft = specialMask | 0x4;
    static constexpr char32_t kInsert = specialMask | 0x5;
    static constexpr char32_t kDelete = specialMask | 0x6;
    static constexpr char32_t kEnd = specialMask | 0x7;
    static constexpr char32_t kHome = specialMask | 0x8;
    static constexpr char32_t kPageUp = specialMask | 0x9;
    static constexpr char32_t kPageDown = specialMask | 0xa;
    static constexpr char32_t kKeypadCenter = specialMask | 0xb;

    static constexpr char32_t kF1 = specialMask | 0x101;
    static constexpr char32_t kF2 = specialMask | 0x102;
    static constexpr char32_t kF3 = specialMask | 0x103;
    static constexpr char32_t kF4 = specialMask | 0x104;
    static constexpr char32_t kF5 = specialMask | 0x105;
    static constexpr char32_t kF6 = specialMask | 0x106;
    static constexpr char32_t kF7 = specialMask | 0x107;
    static constexpr char32_t kF8 = specialMask | 0x108;
    static constexpr char32_t kF9 = specialMask | 0x109;
    static constexpr char32_t kF10 = specialMask | 0x10a;
    static constexpr char32_t kF11 = specialMask | 0x10b;
    static constexpr char32_t kF12 = specialMask | 0x10c;

    static constexpr char32_t kShiftArrowUp = shiftMask | kArrowUp;
    static constexpr char32_t kShiftArrowDown = shiftMask | kArrowDown;
    static constexpr char32_t kShiftArrowRight = shiftMask | kArrowRight;
    static constexpr char32_t kShiftArrowLeft = shiftMask | kArrowLeft;
    static constexpr char32_t kShiftDelete = shiftMask | kDelete;
    static constexpr char32_t kShiftEnd = shiftMask | kEnd;
    static constexpr char32_t kShiftHome = shiftMask | kHome;
    static constexpr char32_t kShiftEnter = shiftMask | 0xfe;
    static constexpr char32_t kShiftTab = shiftMask | 0xff;

    static constexpr char32_t kAltArrowUp = altMask | kArrowUp;
    static constexpr char32_t kAltArrowDown = altMask | kArrowDown;
    static constexpr char32_t kAltArrowRight = altMask | kArrowRight;
    static constexpr char32_t kAltArrowLeft = altMask | kArrowLeft;
    static constexpr char32_t kAltInsert = altMask | kInsert;
    static constexpr char32_t kAltDelete = altMask | kDelete;
    static constexpr char32_t kAltEnd = altMask | kEnd;
    static constexpr char32_t kAltHome = altMask | kHome;
    static constexpr char32_t kAltPageUp = altMask | kPageUp;
    static constexpr char32_t kAltPageDown = altMask | kPageDown;

    static constexpr char32_t kCtrlArrowUp = ctrlMask | kArrowUp;
    static constexpr char32_t kCtrlArrowDown = ctrlMask | kArrowDown;
    static constexpr char32_t kCtrlArrowRight = ctrlMask | kArrowRight;
    static constexpr char32_t kCtrlArrowLeft = ctrlMask | kArrowLeft;
    static constexpr char32_t kCtrlInsert = ctrlMask | kInsert;
    static constexpr char32_t kCtrlDelete = ctrlMask | kDelete;
    static constexpr char32_t kCtrlEnd = ctrlMask | kEnd;
    static constexpr char32_t kCtrlHome = ctrlMask | kHome;
    static constexpr char32_t kCtrlPageUp = ctrlMask | kPageUp;
    static constexpr char32_t kCtrlPageDown = ctrlMask | kPageDown;

    static constexpr Event fromCtrl(char32_t letterOffset)
    {
        return Event{ctrlMask | ('A' + letterOffset)};
    }

    constexpr Event(char32_t v = invalidMask)
        : m_value{v} {}

    explicit operator bool() const
    {
        return (m_value & invalidMask) == 0;
    }

    char32_t value() const
    {
        return m_value;
    }

    auto operator<=>(const Event &) const = default;

private:
    char32_t m_value;
};

/// Color instance
struct Color
{
    // default is a palette color
    static constexpr uint32_t rgbMask = 0x01000000;

    /** Build a color from a palette index.
     * @param[in] paletteIndex  index in the palette
     * @return Color instance
     */
    static constexpr Color fromPalette(uint8_t paletteIndex)
    {
        return Color{paletteIndex};
    }

    /** Build a color from RGB components.
     * @param[in] red    red component
     * @param[in] green  green component
     * @param[in] blue   blue component
     * @return Color instance
     */
    static constexpr Color fromRgb(uint8_t red, uint8_t green, uint8_t blue)
    {
        return Color{rgbMask | ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue};
    }

    /** Build a color from HSV description.
     * @param[in] hue         hue angle [0,360]
     * @param[in] saturation  saturation ratio [0,1]
     * @param[in] value       value ratio [0,1]
     * @return Color instance (encoded in RGB)
     */
    static Color fromHsv(float hue, float saturation, float value);

    constexpr Color(uint32_t v = std::numeric_limits<uint32_t>::max())
        : m_value{v} {}

    /** Whether color is encoded from palette.
     * @return whether color is encoded from palette
     */
    bool isPalette() const
    {
        return (m_value & rgbMask) == 0;
    }

    /** Whether color is encoded from RGB.
     * @return whether color is encoded from RGB
     */
    bool isRgb() const
    {
        return not isPalette();
    }

    /** Get palette index for the color.
     * @note Only valid if isPalette() is true
     * @return palette index
     */
    uint8_t paletteIndex() const
    {
        return m_value;
    }

    /** Get red component for the color.
     * @note Only valid if isRgb() is true
     * @return red component
     */
    uint8_t red() const
    {
        return m_value >> 16;
    }
    /** Get green component for the color.
     * @note Only valid if isRgb() is true
     * @return green component
     */
    uint8_t green() const
    {
        return m_value >> 8;
    }
    /** Get blue component for the color.
     * @note Only valid if isRgb() is true
     * @return blue component
     */
    uint8_t blue() const
    {
        return m_value;
    }

    uint32_t value() const
    {
        return m_value;
    }

    auto operator<=>(const Color &) const = default;

private:
    uint32_t m_value;
};

/// Visual effect on character
struct Effect
{
    // constants aligned with escape sequences
    static constexpr uint32_t kBold = 1 << 1;
    static constexpr uint32_t kItalic = 1 << 3;
    static constexpr uint32_t kUnderline = 1 << 4;
    static constexpr uint32_t kBlink = 1 << 5;
    static constexpr uint32_t kReverseVideo = 1 << 7;
    static constexpr uint32_t kConceal = 1 << 8;
    static constexpr uint32_t kCrossedOut = 1 << 9;

    static constexpr int kFirstBit = 1;
    static constexpr int kLastBit = 9;

    constexpr Effect(uint32_t v = 0)
        : m_value{v} {}

    uint32_t value() const
    {
        return m_value;
    }

    auto operator<=>(const Effect &) const = default;

private:
    uint32_t m_value;
};

/// Handle U32 formatting with invalid unicode values
struct U32Format
{
private:
    /// Use MSB to have an invalid unicode endpoint
    static constexpr char32_t effectMask = 0x40000000;
    static constexpr char32_t colorFgMask = 0x20000000;
    static constexpr char32_t colorBgMask = 0x10000000;
    static constexpr char32_t valueMask = 0x01FFFFF; // unicode on 21 bits
    static constexpr char32_t invalidUnicodeMask = ~valueMask;

public:
    static bool isU32Format(char32_t v)
    {
        return (v & invalidUnicodeMask) != 0;
    }
    static bool isEffect(char32_t v)
    {
        return (v & effectMask) != 0;
    }
    static bool isColorFg(char32_t v)
    {
        return (v & colorFgMask) != 0;
    }
    static bool isColorBg(char32_t v)
    {
        return (v & colorBgMask) != 0;
    }

    static Color getColor(char32_t v)
    {
        return v & valueMask;
    }
    static Effect getEffect(char32_t v)
    {
        return v & valueMask;
    }

    static char32_t buildEffect(Effect effect)
    {
        return effect.value() | effectMask;
    }
    static char32_t buildColorFg(Color color)
    {
        return color.value() | colorFgMask;
    }
    static char32_t buildColorBg(Color color)
    {
        return color.value() | colorBgMask;
    }

    /// Convert Markdown UTF-32 string to U32Format formatted string
    static void convertMarkdown(std::u32string &str);
};

/// Rendering context: color + effect settings to apply to text
struct RenderCtx
{
    Color colorFg; ///< foreground color
    Color colorBg; ///< background color
    Effect effect; ///< text effect
};

/// Represents one cell / glyph on the screen
struct Cell
{
    /** Reset the cell: specified colors, no effect, "space" as glyph
     * @param[in] _colorFg  foreground color
     * @param[in] _colorBg  background color
     */
    void reset(const Color &_colorFg, const Color &_colorBg)
    {
        glyph = U' '; // space as default glyph
        effect = 0;
        colorFg = _colorFg;
        colorBg = _colorBg;
    }

    char32_t glyph; ///< unicode character to draw
    Effect effect;  ///< text effect
    Color colorFg;  ///< foreground color
    Color colorBg;  ///< background color
};

/// Definition of text alignment
struct TextAlignment
{
    static constexpr uint32_t kLeft = 0;
    static constexpr uint32_t kRight = 1;
    static constexpr uint32_t kCentered = 2;
    static constexpr uint32_t kModeMask = 3;
    /// When string is too long, clip the end and add an ellipsis ("too long t…")
    static constexpr uint32_t kClipEnd = 0;
    /// When string is too long, clip the start and add an ellipsis ("…ong text")
    static constexpr uint32_t kClipStart = 1 << 2;

    constexpr TextAlignment(uint32_t v = 0)
        : m_value{v} {}

    bool isClipStart() const
    {
        return (m_value & kClipStart) != 0;
    }

    uint32_t getMode() const
    {
        return m_value & kModeMask;
    }

    uint32_t value() const
    {
        return m_value;
    }

    auto operator<=>(const TextAlignment &) const = default;

private:
    uint32_t m_value;
};

// forward declaration
class TermApp;

/** Terminal Ui instance.
 *
 * Provides access and abstraction of the terminal.
 * 
 * ### Minimal usage
 *
 * - Create a daughter class of termui::MainApp
 *   - implement drawHandler() to draw your app
 *     - use addString*() methods to add content
 *     - at last, call publish() to update the screen
 *   - implement eventHandler() to manage keyboard inputs
 * 
 * - in main()
 *   - instantiate a csys::MainPollHandler
 *   - capture SIGINT, SIGTERM and SIGWINCH signals
 *     ("mainPollHandler.setSignals(SIGINT, SIGTERM, SIGWINCH);")
 *   - instantiate a termui::TermUi
 *   - instantiate your app
 *   - end with return "mainPollHandler.runForever();"
 * 
 * You can also look at the demo for a concrete and complete example.
 * 
 * ### Screen management
 * 
 * TermUi contains a framebuffer, which is a temporary buffer where the application can
 * draw its content before it is displayed. All the addString*() methods modify the framebuffer,
 * not the displayed screen.
 * 
 * Once all the drawing is done, the application shall call publish()
 * to update the screen content.
 * 
 * ### Keyboard management
 * 
 * The terminal sends the pressed key in an inconsistent fashion, so you cannot get all the keys
 * with all the modifiers:
 * - some modifiers are ignored for some keys, like Ctrl+1
 * - some special keys have the same mapping as Ctrl+Key, like Ctrl+M == Enter
 * 
 * Play with the demo to see which keys you can capture.
 * 
 * @note you shall have a single instance active at a time.
 */
class TermUi
{
public:
    TermUi(csys::MainPollHandler &mainPollHandler);
    ~TermUi();

    // not copyable
    TermUi(const TermUi &) = delete;
    TermUi &operator=(const TermUi &) = delete;

    // movable
    TermUi(TermUi &&) noexcept = default;
    TermUi &operator=(TermUi &&) noexcept = default;

    void setTermApp(TermApp *app)
    {
        m_app = app;
    }

    /// Reset frameBuffer (without publishing)
    void reset();

    /** Add one unicode character to the framebuffer.
     * @param[in] y        line / row index, starting from 0
     * @param[in] x        column index, starting from 0
     * @param[in] glyph    unicode character for this cell
     * @param[in] colorFg  foreground color
     * @param[in] colorBg  background color
     * @param[in] effect   text effect
     */
    void addGlyph(int y, int x, char32_t glyph, Color colorFg, Color colorBg, Effect effect = 0);
    void addGlyph(int y, int x, char32_t glyph, Effect effect = 0);
    void addGlyph(int y, int x, char32_t glyph, const RenderCtx &renderCtx);

    /** Add a string to the framebuffer.
     * @param[in] y        line / row index, starting from 0
     * @param[in] x        column index, starting from 0
     * @param[in] str      UTF-8 string
     * @param[in] colorFg  foreground color
     * @param[in] colorBg  background color
     * @param[in] effect   text effect
     */
    void addString(int y, int x, const std::string &str, Color colorFg, Color colorBg, Effect effect = 0);
    void addString(int y, int x, const std::string &str, Effect effect = 0);
    void addString(int y, int x, const std::string &str, const RenderCtx &renderCtx);

    /** Add a string to the framebuffer, with a fixed length.
     * 
     * Display the given string with the given length:
     * - if the string is too long, it is clipped and last character is an ellipsis '…'
     * - if the string is too short, spaces are added to align the text as requested
     * 
     * @param[in] y          line / row index, starting from 0
     * @param[in] x          column index, starting from 0
     * @param[in] str        UTF-8 string
     * @param[in] width      displayed length of the string
     * @param[in] alignment  string alignment in its length
     * @param[in] colorFg    foreground color
     * @param[in] colorBg    background color
     * @param[in] effect     text effect
     */
    void addStringN(int y, int x, const std::string &str, int width, TextAlignment alignment, Color colorFg, Color colorBg, Effect effect = 0);
    void addStringN(int y, int x, const std::string &str, int width, TextAlignment alignment, Effect effect = 0);
    void addStringN(int y, int x, const std::string &str, int width, TextAlignment alignment, const RenderCtx &renderCtx);

    /** Add up to 3 strings to the framebuffer, with a fixed length.
     * 
     * Within the given box (from x to x+width), display the 3 given strings on left, middle and right.
     * Unneeded strings can be set to the empty string.
     * If there is enough space, padding is added between the strings to achieve the wanted length.
     * If there is not enough space, the strings may be clipped (last character will be an ellipsis '…') to avoid overlaps.
     * 
     * @param[in] y          line / row index, starting from 0
     * @param[in] x          column index, starting from 0
     * @param[in] strLeft    UTF-8 string, on left side
     * @param[in] strMiddle  UTF-8 string, in the middle
     * @param[in] strRight   UTF-8 string, on right side
     * @param[in] width      overall length of the strings
     * @param[in] colorFg    foreground color
     * @param[in] colorBg    background color
     * @param[in] effect     text effect
     */
    void addStringsN(int y, int x,
                     const std::string &strLeft, const std::string &strMiddle, const std::string &strRight,
                     int width, Color colorFg, Color colorBg, Effect effect = 0);
    void addStringsN(int y, int x,
                     const std::string &strLeft, const std::string &strMiddle, const std::string &strRight,
                     int width, Effect effect = 0);
    void addStringsN(int y, int x,
                     const std::string &strLeft, const std::string &strMiddle, const std::string &strRight,
                     int width, const RenderCtx &renderCtx);

    /** Add a formatted UTF-32 string to the framebuffer, with a maximum length.
     * 
     * Display the given string, limited to the given maximum length.
     * The string may contain special formatting values generated with U32Format:
     * they are interpreted to modify the graphic rendering (effect, fg/bg color).
     * 
     * @param[in] y             line / row index, starting from 0
     * @param[in] x             column index, starting from 0
     * @param[in] formattedStr  formatted UTF-32 string
     * @param[in] width         display width used by the string
     */
    void addFString(int y, int x, const std::u32string &formattedStr, int width);

    /** Add a markdown text to the framebuffer, with a maximum length.
     * 
     * Display the given text, limited to the given maximum length.
     * The string may contain multiple lines, with markdown formatting.
     *
     * @param[in] y             line / row index, starting from 0
     * @param[in] x             column index, starting from 0
     * @param[in] str           markdown text
     * @param[in] width         display width used by the string
     */
    void addMarkdown(int y, int x, const std::string &str, int width);

    /// Publish the frameBuffer content to the screen
    void publish();

    /** Terminal width.
     * @return current terminal width (number of columns)
     */
    int width() const
    {
        return m_tty.width;
    }

    /** Terminal height.
     * @return current terminal height (number of lines or rows)
     */
    int height() const
    {
        return m_tty.height;
    }

    /** Set default terminal colors.
     * @note a reset() shall be done after to take this change into account.
     * @param[in] colorFg    foreground color
     * @param[in] colorBg    background color
     */
    void setDefaultColors(Color colorFg, Color colorBg)
    {
        m_colorFg = colorFg;
        m_colorBg = colorBg;
    }

    /** Set colors for a range of cells.
     * @param[in] y          line / row index, starting from 0
     * @param[in] x          column index, starting from 0
     * @param[in] width      number of cells to set color
     * @param[in] colorFg    foreground color
     * @param[in] colorBg    background color
     */
    void setColors(int y, int x, int width, Color colorFg, Color colorBg);

private:
    /// Handle event from m_tty
    void readTtyHandler(uint32_t events);

    /// Handle signal for terminal resize
    void resizeSigHandler();

    /** Add a UTF-32 standard string to the framebuffer.
     * @param[in] y        line / row index, starting from 0
     * @param[in] x        column index, starting from 0
     * @param[in] strU32   UTF-32 string
     * @param[in] colorFg  foreground color
     * @param[in] colorBg  background color
     * @param[in] effect   text effect
     */
    void addStdU32String(int y, int x, const std::u32string &strU32, Color colorFg, Color colorBg, Effect effect = 0);

    /// Update fg / bgcolor (partial graphic command)
    void updateColorSetting(Color color, bool isFg);

    /// Update graphic settings based on the current state
    void updateGraphicSettings(
        Effect &currentEffect,
        Color &currentFg,
        Color &currentBg,
        Effect wantedEffect,
        Color wantedFg,
        Color wantedBg);

    TermApp *m_app;                    ///< user application
    internal::ScopedBufferedTty m_tty; ///< tty handler
    std::vector<Cell> m_frameBuffer;   ///< store / preparation of next screen content
    bool m_dirty;                      ///< whether m_frameBuffer contains unpublished modifications
    Color m_colorFg;                   ///< screen default foreground color
    Color m_colorBg;                   ///< screen default background color
};

inline void TermUi::addGlyph(int y, int x, char32_t glyph, Effect effect)
{
    addGlyph(y, x, glyph, m_colorFg, m_colorBg, effect);
}
inline void TermUi::addGlyph(int y, int x, char32_t glyph, const RenderCtx &renderCtx)
{
    addGlyph(y, x, glyph, renderCtx.colorFg, renderCtx.colorBg, renderCtx.effect);
}
inline void TermUi::addString(int y, int x, const std::string &str, Effect effect)
{
    addString(y, x, str, m_colorFg, m_colorBg, effect);
}
inline void TermUi::addString(int y, int x, const std::string &str, const RenderCtx &renderCtx)
{
    addString(y, x, str, renderCtx.colorFg, renderCtx.colorBg, renderCtx.effect);
}
inline void TermUi::addStringN(int y, int x, const std::string &str, int width, TextAlignment alignment, Effect effect)
{
    addStringN(y, x, str, width, alignment, m_colorFg, m_colorBg, effect);
}
inline void TermUi::addStringN(int y, int x, const std::string &str, int width, TextAlignment alignment, const RenderCtx &renderCtx)
{
    addStringN(y, x, str, width, alignment, renderCtx.colorFg, renderCtx.colorBg, renderCtx.effect);
}
inline void TermUi::addStringsN(int y, int x,
                                const std::string &strLeft, const std::string &strMiddle, const std::string &strRight,
                                int width, Effect effect)
{
    addStringsN(y, x, strLeft, strMiddle, strRight, width, m_colorFg, m_colorBg, effect);
}
inline void TermUi::addStringsN(int y, int x,
                                const std::string &strLeft, const std::string &strMiddle, const std::string &strRight,
                                int width, const RenderCtx &renderCtx)
{
    addStringsN(y, x, strLeft, strMiddle, strRight, width, renderCtx.colorFg, renderCtx.colorBg, renderCtx.effect);
}

inline void TermUi::addGlyph(int y, int x, char32_t glyph, Color colorFg, Color colorBg, Effect effect)
{
    if (x >= 0 and x < m_tty.width and y >= 0 and y < m_tty.height)
    {
        auto &cell = m_frameBuffer[y * m_tty.width + x];
        cell.colorFg = colorFg;
        cell.colorBg = colorBg;
        cell.effect = effect;
        cell.glyph = glyph;
        m_dirty = true;
    }
}

inline void TermUi::addString(int y, int x, const std::string &str, Color colorFg, Color colorBg, Effect effect)
{
    // convert the UTF8 string to glyphs
    const std::u32string strU32 = toU32String(str);
    // process the string
    addStdU32String(y, x, strU32, colorFg, colorBg, effect);
}

inline void TermUi::setColors(int y, int x, int width, Color colorFg, Color colorBg)
{
    if (x >= 0 and x < m_tty.width and y >= 0 and y < m_tty.height)
    {
        auto *cellPtr = &m_frameBuffer[y * m_tty.width + x];
        for (int i = 0; i < std::min(width, m_tty.width - x); i++, cellPtr++)
        {
            cellPtr->colorFg = colorFg;
            cellPtr->colorBg = colorBg;
        }
        m_dirty = true;
    }
}

/// Base class for terminal application
class TermApp
{
public:
    TermApp(TermUi &term)
        : m_term{term}
    {
        m_term.setTermApp(this);
    }

    // not copyable
    TermApp(const TermApp &) = delete;
    TermApp &operator=(const TermApp &) = delete;

    // not movable
    TermApp(TermApp &&) noexcept = delete;
    TermApp &operator=(TermApp &&) noexcept = delete;

    /// Request complete drawing of the application
    virtual void drawHandler() = 0;

    /// Process keyboard event
    virtual void eventHandler(Event event) = 0;

protected:
    TermUi &m_term; ///< termui handler
};

} // namespace termui