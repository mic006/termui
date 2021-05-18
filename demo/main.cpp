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
 * main() function of termui demo.
 */

#include <iostream>
#include <locale>
#include <stdlib.h>

#include "csys.h"
#include "termui.h"

using namespace termui;
using namespace std::string_literals;

static csys::MainPollHandler mainPollHandler{};

class MainApp : protected termui::TermApp
{
public:
    MainApp(TermUi &term);
    void drawHandler() override;
    void eventHandler(Event event) override;

protected:
    enum class DemoScreen
    {
        Welcome,
        TextEffect,
        Palette,
        RgbFg,
        RgbBg,
        Keyboard,
        Doc,
    };

    std::string drawWelcomeScreen();
    std::string drawTextEffectScreen();
    std::string drawPaletteScreen();
    std::string drawRgbScreen(bool isFg);
    std::string drawKeyboardScreen();
    std::string drawDocScreen();

    DemoScreen m_screen; ///< current screen
    char32_t m_glyph;    ///< glyph used to fill RGB screens
    Event m_lastEvent{}; ///< last event for keyboard screen
};

MainApp::MainApp(TermUi &term)
    : termui::TermApp{term},
      m_screen{DemoScreen::Welcome},
      m_glyph{'X'}

{
    drawHandler();
}

std::string MainApp::drawWelcomeScreen()
{
    int line = 2;
    m_term.addString(line++, 0, "You can use the following keys to go through the demo:");
    m_term.addString(line++, 0, "- Esc / q / Ctrl+C : quit the demo");
    m_term.addString(line++, 0, "- 0 / h : this help screen");
    m_term.addString(line++, 0, "- 1 : text effects.");
    m_term.addString(line++, 0, "- 2 : palette colors.");
    m_term.addString(line++, 0, "- 3 : RGB gradient foreground color. Press any key to change the character used.");
    m_term.addString(line++, 0, "- 4 : RGB gradient background color. Press any key to change the character used.");
    m_term.addString(line++, 0, "- 5 : keyboard / event demo: display the captured events.");
    m_term.addString(line++, 0, "- 6 : extract of the API documentation.");
    line++;
    m_term.addString(line++, 0, "You can also resize the window at any moment to see the refresh.");

    return "TermUI demo";
}

std::string MainApp::drawTextEffectScreen()
{
    int line = 2;
    m_term.addString(line++, 0, "With default color, normal text");
    m_term.addString(line++, 0, "Bold text (may appear brighter)", Effect::kBold);
    m_term.addString(line++, 0, "Italic text", Effect::kItalic);
    m_term.addString(line++, 0, "Underline text", Effect::kUnderline);
    m_term.addString(line++, 0, "Blinking text", Effect::kBlink);
    m_term.addString(line++, 0, "Reversed-video text", Effect::kReverseVideo);
    m_term.addString(line++, 0, "Concealed text", Effect::kConceal);
    m_term.addString(line++, 0, "Crossed-out text", Effect::kCrossedOut);
    line++;

    Color black = Color::fromPalette(0);
    Color color = Color::fromPalette(27);
    m_term.addString(line++, 0, "With fixed foreground color, normal text", color, black);
    m_term.addString(line++, 0, "Bold text (not brighter as color is fixed)", color, black, Effect::kBold);
    m_term.addString(line++, 0, "Italic text", color, black, Effect::kItalic);
    m_term.addString(line++, 0, "Underline text", color, black, Effect::kUnderline);
    m_term.addString(line++, 0, "Blinking text", color, black, Effect::kBlink);
    m_term.addString(line++, 0, "Reversed-video text", color, black, Effect::kReverseVideo);
    m_term.addString(line++, 0, "Concealed text", color, black, Effect::kConceal);
    m_term.addString(line++, 0, "Crossed-out text", color, black, Effect::kCrossedOut);

    return "TermUI demo - Text effect";
}

std::string MainApp::drawPaletteScreen()
{
    int line = 2;
    const int blockWidth = 6;
    Color black = Color::fromPalette(0);
    Color white = Color::fromPalette(15);

    m_term.addString(line++, 0, "Standard colors");
    for (int c = 0; c < 8; c++)
    {
        const int paletteIndex = c;
        m_term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, white, Color::fromPalette(paletteIndex));
    }
    line += 2;

    m_term.addString(line++, 0, "High-intensity colors");
    for (int c = 0; c < 8; c++)
    {
        const int paletteIndex = 8 + c;
        m_term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, black, Color::fromPalette(paletteIndex));
    }
    line += 2;

    m_term.addString(line++, 0, "216 colors");
    for (int y = 0; y < 6; y++, line++)
    {
        for (int c = 0; c < 18; c++)
        {
            const int paletteIndex = 16 + 36 * y + c;
            m_term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, white, Color::fromPalette(paletteIndex));
        }
    }
    for (int y = 0; y < 6; y++, line++)
    {
        for (int c = 0; c < 18; c++)
        {
            const int paletteIndex = 16 + 36 * y + 18 + c;
            m_term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, black, Color::fromPalette(paletteIndex));
        }
    }
    line++;

    m_term.addString(line++, 0, "24 grey shades");
    for (int c = 0; c < 12; c++)
    {
        const int paletteIndex = 232 + c;
        m_term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, white, Color::fromPalette(paletteIndex));
    }
    line++;
    for (int c = 0; c < 12; c++)
    {
        const int paletteIndex = 244 + c;
        m_term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, black, Color::fromPalette(paletteIndex));
    }
    line++;

    return "TermUI demo - Color palette";
}

std::string MainApp::drawRgbScreen(bool isFg)
{
    const int width = m_term.width();
    const int height = m_term.height();
    float hueStep = 360.0 / width;
    float valueStep = 1.0 / (height - 2);
    Color black = Color::fromRgb(0, 0, 0);

    for (int line = 1; line < height - 1; line++)
    {
        for (int x = 0; x < width; x++)
        {
            Color color = Color::fromHsv(x * hueStep, 1.0, 1.0 - (line - 1) * valueStep);
            m_term.addGlyph(line, x, m_glyph, isFg ? color : black, isFg ? black : color);
        }
    }

    return "TermUI demo - RBG palette";
}

std::string MainApp::drawKeyboardScreen()
{
    static const char *SpecialKeys[] = {
        "<ArrowUp>",
        "<ArrowDown>",
        "<ArrowRight>",
        "<ArrowLeft>",
        "<Insert>",
        "<Delete>",
        "<End>",
        "<Home>",
        "<PageUp>",
        "<PageDown>",
        "<KeypadCenter>",
    };

    int line = 2;
    const char32_t value = m_lastEvent.value();
    std::string eventDesc = "";
    bool displayGlyph = false;
    // look for some special events
    switch (value)
    {
    case Event::kBackspace:
        eventDesc = "<BackSpace>";
        break;
    case Event::kTab:
        eventDesc = "<Tab> (or [Ctrl] I)";
        break;
    case Event::kEnter:
        eventDesc = "<Enter> (or [Ctrl] M)";
        break;
    case Event::kEscape:
        eventDesc = "<Escape>";
        break;
    case Event::kShiftEnter:
        eventDesc = "[Shift] <Enter>";
        break;
    case Event::kShiftTab:
        eventDesc = "[Shift] <Tab>";
        break;
    case ' ':
        eventDesc = "<Space>";
        break;
    default:
        // all other keys
        if ((value & Event::invalidMask) != 0)
        {
            eventDesc = "Invalid !";
        }
        else
        {
            if ((value & Event::ctrlMask) != 0)
                eventDesc += "[Ctrl] ";
            if ((value & Event::altMask) != 0)
                eventDesc += "[Alt] ";
            if ((value & Event::shiftMask) != 0)
                eventDesc += "[Shift] ";
            if ((value & Event::specialMask) != 0)
            {
                const char32_t baseValue = value & Event::valueMask;
                if (baseValue >= 1 and baseValue <= 0xb)
                    eventDesc += SpecialKeys[baseValue - 1];
                else if (baseValue >= 0x101 and baseValue <= 0x10c)
                    // function key
                    eventDesc += "<F" + std::to_string(baseValue - 0x100) + ">";
                else
                    eventDesc += "<Unknown>";
            }
            else
            {
                // normal glyph
                displayGlyph = true;
            }
        }
        break;
    }

    m_term.addString(line++, 0, "Press any key (or key combination) to see the associated event.");
    line++;
    const std::string str = "Last event: " + eventDesc;
    m_term.addString(line, 0, str);
    if (displayGlyph)
        m_term.addGlyph(line, str.size(), m_lastEvent.value() & Event::valueMask);

    return "TermUI demo - Keyboard / capture events";
}

std::string MainApp::drawDocScreen()
{
    const int width = m_term.width();

    m_term.addMarkdown(1,
                       0,
                       R"(**Minimal usage**
- Create a daughter class of //termui::MainApp//
  - implement //drawHandler()// to draw your app
    - use //addString*()// methods to add content
    - at last, call //publish()// to update the screen
  - implement //eventHandler()// to manage keyboard inputs

- in //main()//
  - instantiate a //csys::MainPollHandler//
  - capture SIGINT, SIGTERM and SIGWINCH signals ("//mainPollHandler.setSignals(SIGINT, SIGTERM, SIGWINCH);//")
  - instantiate a //termui::TermUi//
  - instantiate your app
  - end with return "//mainPollHandler.runForever();//"
                                                                                                                                                   
**Drawing APIs**
- //addGlyph//: add a single unicode character at the given position
- //addString//: add a UTF-8 string, starting at the given position
- //addStringN//: add a UTF-8 string with a fixed length, with alignment and clipping options
- //addStringsN//: add 3 UTF-8 strings as left / middle / right in a given length (used to display the footer)
- //addFString//: add a UTF-32 string with special formatting values to change colors / effects in the middle
- //addMarkdown//: add markdown text to the screen, with basic formatting
  (2* for **bold**, 2/ for //italic//, 2_ for __underline__, 2- for --crossed-out--)
)",
                       width);

    return "TermUI demo - extract of API doc";
}

void MainApp::drawHandler()
{
    m_term.reset();
    std::string title;
    switch (m_screen)
    {
    case DemoScreen::Welcome:
        title = drawWelcomeScreen();
        break;
    case DemoScreen::TextEffect:
        title = drawTextEffectScreen();
        break;
    case DemoScreen::Palette:
        title = drawPaletteScreen();
        break;
    case DemoScreen::RgbFg:
        title = drawRgbScreen(true);
        break;
    case DemoScreen::RgbBg:
        title = drawRgbScreen(false);
        break;
    case DemoScreen::Keyboard:
        title = drawKeyboardScreen();
        break;
    case DemoScreen::Doc:
        title = drawDocScreen();
        break;
    }
    // add title and footer
    m_term.addStringN(0, 0, title, m_term.width(), TextAlignment::kCentered, Effect::kReverseVideo);
    m_term.addStringsN(m_term.height() - 1, 0, " q / Ctrl+c to quit", "", "F1 / h for help ", m_term.width(), Effect::kReverseVideo);

    m_term.publish();
}

void MainApp::eventHandler(Event event)
{
    m_lastEvent = event;

    switch (event.value())
    {
    case Event::kCtrlC:
    case Event::kEscape:
    case 'q':
    case 'Q':
        mainPollHandler.requestTermination();
        break;

    case Event::kF1:
    case 'h':
    case 'H':
    case '0':
        m_screen = DemoScreen::Welcome;
        break;

    case '1':
        m_screen = DemoScreen::TextEffect;
        break;

    case '2':
        m_screen = DemoScreen::Palette;
        break;

    case '3':
        m_screen = DemoScreen::RgbFg;
        m_glyph = 'X';
        break;

    case '4':
        m_screen = DemoScreen::RgbBg;
        m_glyph = 'X';
        break;

    case '5':
        m_screen = DemoScreen::Keyboard;
        break;

    case '6':
        m_screen = DemoScreen::Doc;
        break;

    default:
        if ((event.value() & (Event::specialMask | Event::invalidMask)) == 0)
        {
            // printable character
            m_glyph = event.value() & Event::valueMask;
        }
        break;
    }

    // redraw screen
    drawHandler();
}

int main()
{
    std::locale::global(std::locale(""));
    mainPollHandler.setSignals(SIGINT, SIGTERM, SIGWINCH);
    TermUi term{mainPollHandler};
    MainApp mainapp{term};

    return mainPollHandler.runForever();
}
