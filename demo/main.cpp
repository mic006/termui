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

#include "termui.h"

using namespace termui;
using namespace std::string_literals;

static std::string drawWelcomeScreen(TermUi &term)
{
    int line = 2;
    term.addString(line++, 0, "You can use the following keys to go through the demo:");
    term.addString(line++, 0, "- Esc / q / Ctrl+C : quit the demo");
    term.addString(line++, 0, "- 0 / h : this help screen");
    term.addString(line++, 0, "- 1 : text effects.");
    term.addString(line++, 0, "- 2 : palette colors.");
    term.addString(line++, 0, "- 3 : RGB gradient foreground color. Press any key to change the character used.");
    term.addString(line++, 0, "- 4 : RGB gradient background color. Press any key to change the character used.");
    term.addString(line++, 0, "- 5 : keyboard / event demo: display the captured events.");
    term.addString(line++, 0, "- 6 : extract of the API documentation.");
    line++;
    term.addString(line++, 0, "You can also resize the window at any moment to see the refresh.");

    return "TermUI demo";
}

static std::string drawTextEffectScreen(TermUi &term)
{
    int line = 2;
    term.addString(line++, 0, "With default color, normal text");
    term.addString(line++, 0, "Bold text (may appear brighter)", Effect::kBold);
    term.addString(line++, 0, "Italic text", Effect::kItalic);
    term.addString(line++, 0, "Underline text", Effect::kUnderline);
    term.addString(line++, 0, "Blinking text", Effect::kBlink);
    term.addString(line++, 0, "Reversed-video text", Effect::kReverseVideo);
    term.addString(line++, 0, "Concealed text", Effect::kConceal);
    term.addString(line++, 0, "Crossed-out text", Effect::kCrossedOut);
    line++;

    Color black = Color::fromPalette(0);
    Color color = Color::fromPalette(27);
    term.addString(line++, 0, "With fixed foreground color, normal text", color, black);
    term.addString(line++, 0, "Bold text (not brighter as color is fixed)", color, black, Effect::kBold);
    term.addString(line++, 0, "Italic text", color, black, Effect::kItalic);
    term.addString(line++, 0, "Underline text", color, black, Effect::kUnderline);
    term.addString(line++, 0, "Blinking text", color, black, Effect::kBlink);
    term.addString(line++, 0, "Reversed-video text", color, black, Effect::kReverseVideo);
    term.addString(line++, 0, "Concealed text", color, black, Effect::kConceal);
    term.addString(line++, 0, "Crossed-out text", color, black, Effect::kCrossedOut);

    return "TermUI demo - Text effect";
}

static std::string drawPaletteScreen(TermUi &term)
{
    int line = 2;
    const int blockWidth = 6;
    Color black = Color::fromPalette(0);
    Color white = Color::fromPalette(15);

    term.addString(line++, 0, "Standard colors");
    for (int c = 0; c < 8; c++)
    {
        const int paletteIndex = c;
        term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, white, Color::fromPalette(paletteIndex));
    }
    line += 2;

    term.addString(line++, 0, "High-intensity colors");
    for (int c = 0; c < 8; c++)
    {
        const int paletteIndex = 8 + c;
        term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, black, Color::fromPalette(paletteIndex));
    }
    line += 2;

    term.addString(line++, 0, "216 colors");
    for (int y = 0; y < 6; y++, line++)
    {
        for (int c = 0; c < 18; c++)
        {
            const int paletteIndex = 16 + 36 * y + c;
            term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, white, Color::fromPalette(paletteIndex));
        }
    }
    for (int y = 0; y < 6; y++, line++)
    {
        for (int c = 0; c < 18; c++)
        {
            const int paletteIndex = 16 + 36 * y + 18 + c;
            term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, black, Color::fromPalette(paletteIndex));
        }
    }
    line++;

    term.addString(line++, 0, "24 grey shades");
    for (int c = 0; c < 12; c++)
    {
        const int paletteIndex = 232 + c;
        term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, white, Color::fromPalette(paletteIndex));
    }
    line++;
    for (int c = 0; c < 12; c++)
    {
        const int paletteIndex = 244 + c;
        term.addStringN(line, c * blockWidth, std::to_string(paletteIndex), blockWidth, TextAlignment::kCentered, black, Color::fromPalette(paletteIndex));
    }
    line++;

    return "TermUI demo - Color palette";
}

static std::string drawRgbScreen(TermUi &term, char32_t glyph, bool isFg)
{
    const int width = term.width();
    const int height = term.height();
    float hueStep = 360.0 / width;
    float valueStep = 1.0 / (height - 2);
    Color black = Color::fromRgb(0, 0, 0);

    for (int line = 1; line < height - 1; line++)
    {
        for (int x = 0; x < width; x++)
        {
            Color color = Color::fromHsv(x * hueStep, 1.0, 1.0 - (line - 1) * valueStep);
            term.addGlyph(line, x, glyph, isFg ? color : black, isFg ? black : color);
        }
    }

    return "TermUI demo - RBG palette";
}

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

static std::string drawKeyboardScreen(TermUi &term, Event lastEvent)
{
    int line = 2;
    const char32_t value = lastEvent.value();
    std::string eventDesc = "";
    bool displayGlyph = false;
    // look for some special events
    switch (value)
    {
    case Event::kSigInt:
        eventDesc = "[Signal] SIGINT";
        break;
    case Event::kSigTerm:
        eventDesc = "[Signal] SIGTERM";
        break;
    case Event::kTermResize:
        eventDesc = "[Signal] SIGWINCH = Terminal resize";
        break;
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

    term.addString(line++, 0, "Press any key (or key combination) to see the associated event.");
    line++;
    const std::string str = "Last event: " + eventDesc;
    term.addString(line, 0, str);
    if (displayGlyph)
        term.addGlyph(line, str.size(), lastEvent.value() & Event::valueMask);

    return "TermUI demo - Keyboard / capture events";
}

static std::string drawDocScreen(TermUi &term)
{
    const int width = term.width();
    char32_t italic = U32Format::buildEffect(Effect::kItalic);
    char32_t normal = U32Format::buildEffect(0);

    int line = 2;
    term.addString(line++, 0, "Minimal usage", Effect::kBold);
    term.addFString(line++, 0, U"- instantiate a "s + italic + U"TermUi", width);
    term.addFString(line++, 0, U"- use the "s + italic + U"addString*()" + normal + U" methods to add content to the screen", width);
    term.addFString(line++, 0, U"- call "s + italic + U"waitForEvent()" + normal + U" to wait for user interaction", width);
    term.addString(line++, 0, "- handle at least the following events:");
    term.addFString(line++, 0, U"  - "s + italic + U"kCtrlC" + normal + U", "s + italic + U"kSigInt" + normal + U", "s + italic + U"kSigTerm" + normal + U" to quit the application", width);
    term.addFString(line++, 0, U"  - "s + italic + U"kTermResize" + normal + U" to redraw the screen based on its new size", width);
    line++;
    term.addString(line++, 0, "Important APIs", Effect::kBold);
    term.addFString(line++, 0, U"- "s + italic + U"addGlyph" + normal + U": add a single unicode character at the given position", width);
    term.addFString(line++, 0, U"- "s + italic + U"addString" + normal + U": add a UTF-8 string, starting at the given position", width);
    term.addFString(line++, 0, U"- "s + italic + U"addStringN" + normal + U": add a UTF-8 string with a fixed length, with alignment and clipping options", width);
    term.addFString(line++, 0, U"- "s + italic + U"addStringsN" + normal + U": add 3 UTF-8 strings as left / middle / right in a given length", width);
    term.addString(line++, 0, "  (used to display the footer)");
    term.addFString(line++, 0, U"- "s + italic + U"addFString" + normal + U": add a UTF-32 string with special formatting values to change colors / effects in the middle", width);
    term.addString(line++, 0, "  (used to display most lines of this list)");
    term.addFString(line++, 0, U"- "s + italic + U"waitForEvent" + normal + U": publish frame buffer content to the screen and wait for an event (keyboard, signal, resize)", width);

    return "TermUI demo - extract of API doc";
}

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

int main()
{
    std::locale::global(std::locale(""));
    TermUi term{};
    DemoScreen screen = DemoScreen::Welcome;
    char32_t glyph = 'X';
    bool exit = false;
    Event lastEvent{};

    while (not exit)
    {
        term.reset();
        std::string title;
        switch (screen)
        {
        case DemoScreen::Welcome:
            title = drawWelcomeScreen(term);
            break;
        case DemoScreen::TextEffect:
            title = drawTextEffectScreen(term);
            break;
        case DemoScreen::Palette:
            title = drawPaletteScreen(term);
            break;
        case DemoScreen::RgbFg:
            title = drawRgbScreen(term, glyph, true);
            break;
        case DemoScreen::RgbBg:
            title = drawRgbScreen(term, glyph, false);
            break;
        case DemoScreen::Keyboard:
            title = drawKeyboardScreen(term, lastEvent);
            break;
        case DemoScreen::Doc:
            title = drawDocScreen(term);
            break;
        }
        // add title and footer
        term.addStringN(0, 0, title, term.width(), TextAlignment::kCentered, Effect::kReverseVideo);
        term.addStringsN(term.height() - 1, 0, " q / Ctrl+c to quit", "", "F1 / h for help ", term.width(), Effect::kReverseVideo);

        const Event event = term.waitForEvent();
        if (false)
        {
            // log event to stderr
            const uint32_t v = event.value();
            std::cerr << "Event: ";
            std::cerr << std::hex << v;
            if (v >= 32 and v < 128)
                std::cerr << " " << (char)v;
            std::cerr << std::endl;
        }
        lastEvent = event;

        switch (event.value())
        {
        case Event::kTermResize:
            break;

        case Event::kSigInt:
        case Event::kSigTerm:
        case Event::kCtrlC:
        case Event::kEscape:
        case 'q':
        case 'Q':
            exit = true;
            break;

        case Event::kF1:
        case 'h':
        case 'H':
        case '0':
            screen = DemoScreen::Welcome;
            break;

        case '1':
            screen = DemoScreen::TextEffect;
            break;

        case '2':
            screen = DemoScreen::Palette;
            break;

        case '3':
            screen = DemoScreen::RgbFg;
            glyph = 'X';
            break;

        case '4':
            screen = DemoScreen::RgbBg;
            glyph = 'X';
            break;

        case '5':
            screen = DemoScreen::Keyboard;
            break;

        case '6':
            screen = DemoScreen::Doc;
            break;

        default:
            if ((event.value() & (Event::specialMask | Event::invalidMask | Event::signalMask)) == 0)
            {
                // printable character
                glyph = event.value() & Event::valueMask;
            }
            break;
        }
    }

    return EXIT_SUCCESS;
}