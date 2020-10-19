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
 * Identify a known input escape sequence.
 */

#ifdef TERMUI_INPUT_ESC_SEQ_USE_SEARCH
#include <cstring>
#include <string>
#else
#include <cstddef>
#endif

namespace termui
{
static char32_t identify_esc_seq(const char *data, size_t size, size_t &consumed);

#ifdef TERMUI_INPUT_ESC_SEQ_USE_SEARCH
/* Search implementation.
 * Go through the list of known sequences and compare one-by-one
 */

/// Definition for one key
struct KeyDefinition
{
    char32_t event;     ///< associated event
    std::string escSeq; ///< escape sequence sent by terminal
};

/// Mapping for keys and escape sequences
static const KeyDefinition keyDefinitions[] = {
    {Event::kArrowUp, "OA"},
    {Event::kArrowDown, "OB"},
    {Event::kArrowRight, "OC"},
    {Event::kArrowLeft, "OD"},
    {Event::kInsert, "[2~"},
    {Event::kDelete, "[3~"},
    {Event::kEnd, "OF"},
    {Event::kHome, "OH"},
    {Event::kPageUp, "[5~"},
    {Event::kPageDown, "[6~"},
    {Event::kKeypadCenter, "[E"},

    {Event::kF1, "OP"},
    {Event::kF2, "OQ"},
    {Event::kF3, "OR"},
    {Event::kF4, "OS"},
    {Event::kF5, "[15~"},
    {Event::kF6, "[17~"},
    {Event::kF7, "[18~"},
    {Event::kF8, "[19~"},
    {Event::kF9, "[20~"},
    {Event::kF10, "[21~"},
    {Event::kF11, "[23~"},
    {Event::kF12, "[24~"},

    {Event::kShiftArrowUp, "[1;2A"},
    {Event::kShiftArrowDown, "[1;2B"},
    {Event::kShiftArrowRight, "[1;2C"},
    {Event::kShiftArrowLeft, "[1;2D"},
    {Event::kShiftDelete, "[3;2~"},
    {Event::kShiftEnd, "[1;2F"},
    {Event::kShiftHome, "[1;2H"},
    {Event::kShiftEnter, "OM"},
    {Event::kShiftTab, "[Z"},

    {Event::kAltArrowUp, "[1;1A"},
    {Event::kAltArrowDown, "[1;1B"},
    {Event::kAltArrowRight, "[1;1C"},
    {Event::kAltArrowLeft, "[1;1D"},
    {Event::kAltInsert, "[2;1~"},
    {Event::kAltDelete, "[3;1~"},
    {Event::kAltEnd, "[1;1F"},
    {Event::kAltHome, "[1;1H"},
    {Event::kAltPageUp, "[5;1~"},
    {Event::kAltPageDown, "[6;1~"},

    {Event::kCtrlArrowUp, "[1;5A"},
    {Event::kCtrlArrowDown, "[1;5B"},
    {Event::kCtrlArrowRight, "[1;5C"},
    {Event::kCtrlArrowLeft, "[1;5D"},
    {Event::kCtrlInsert, "[2;5~"},
    {Event::kCtrlDelete, "[3;5~"},
    {Event::kCtrlEnd, "[1;5F"},
    {Event::kCtrlHome, "[1;5H"},
    {Event::kCtrlPageUp, "[5;5~"},
    {Event::kCtrlPageDown, "[6;5~"},
};

char32_t identify_esc_seq(const char *data, size_t size, size_t &consumed)
{
    for (const auto &keydef : keyDefinitions)
    {
        if (keydef.escSeq.size() <= size and
            std::memcmp(keydef.escSeq.data(), data, keydef.escSeq.size()) == 0)
        {
            // sequence found
            consumed = keydef.escSeq.size();
            return keydef.event;
        }
    }
    return 0;
}

#else
/* Generated parser implementation.
 * Use a kind of graph walking / state machine to look at each character once.
 * The code is generated from the list of wanted keys.
 * The compiler can optimize it quite well with lookup tables.
 * Benchmark with -O3: this one is 25x faster than search above.
 * 
 * DO NOT MODIFY by hand, use `termui_input_esc_seq_gene.py` to modify
 * the list of expected keys and generate the matching code.
 */
char32_t identify_esc_seq(const char *data, size_t size, size_t &consumed)
{
    if (size >= 1)
    {
        switch (data[0])
        {
        case 'O':
            if (size >= 2)
            {
                switch (data[1])
                {
                case 'A':
                    consumed = 2;
                    return Event::kArrowUp;
                case 'B':
                    consumed = 2;
                    return Event::kArrowDown;
                case 'C':
                    consumed = 2;
                    return Event::kArrowRight;
                case 'D':
                    consumed = 2;
                    return Event::kArrowLeft;
                case 'F':
                    consumed = 2;
                    return Event::kEnd;
                case 'H':
                    consumed = 2;
                    return Event::kHome;
                case 'P':
                    consumed = 2;
                    return Event::kF1;
                case 'Q':
                    consumed = 2;
                    return Event::kF2;
                case 'R':
                    consumed = 2;
                    return Event::kF3;
                case 'S':
                    consumed = 2;
                    return Event::kF4;
                case 'M':
                    consumed = 2;
                    return Event::kShiftEnter;
                default:
                    break;
                }
            }
            break;
        case '[':
            if (size >= 2)
            {
                switch (data[1])
                {
                case '2':
                    if (size >= 3)
                    {
                        switch (data[2])
                        {
                        case '~':
                            consumed = 3;
                            return Event::kInsert;
                        case '0':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '~':
                                    consumed = 4;
                                    return Event::kF9;
                                default:
                                    break;
                                }
                            }
                            break;
                        case '1':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '~':
                                    consumed = 4;
                                    return Event::kF10;
                                default:
                                    break;
                                }
                            }
                            break;
                        case '3':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '~':
                                    consumed = 4;
                                    return Event::kF11;
                                default:
                                    break;
                                }
                            }
                            break;
                        case '4':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '~':
                                    consumed = 4;
                                    return Event::kF12;
                                default:
                                    break;
                                }
                            }
                            break;
                        case ';':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '1':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case '~':
                                            consumed = 5;
                                            return Event::kAltInsert;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                case '5':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case '~':
                                            consumed = 5;
                                            return Event::kCtrlInsert;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case '3':
                    if (size >= 3)
                    {
                        switch (data[2])
                        {
                        case '~':
                            consumed = 3;
                            return Event::kDelete;
                        case ';':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '2':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case '~':
                                            consumed = 5;
                                            return Event::kShiftDelete;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                case '1':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case '~':
                                            consumed = 5;
                                            return Event::kAltDelete;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                case '5':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case '~':
                                            consumed = 5;
                                            return Event::kCtrlDelete;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case '5':
                    if (size >= 3)
                    {
                        switch (data[2])
                        {
                        case '~':
                            consumed = 3;
                            return Event::kPageUp;
                        case ';':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '1':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case '~':
                                            consumed = 5;
                                            return Event::kAltPageUp;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                case '5':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case '~':
                                            consumed = 5;
                                            return Event::kCtrlPageUp;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case '6':
                    if (size >= 3)
                    {
                        switch (data[2])
                        {
                        case '~':
                            consumed = 3;
                            return Event::kPageDown;
                        case ';':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '1':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case '~':
                                            consumed = 5;
                                            return Event::kAltPageDown;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                case '5':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case '~':
                                            consumed = 5;
                                            return Event::kCtrlPageDown;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case 'E':
                    consumed = 2;
                    return Event::kKeypadCenter;
                case '1':
                    if (size >= 3)
                    {
                        switch (data[2])
                        {
                        case '5':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '~':
                                    consumed = 4;
                                    return Event::kF5;
                                default:
                                    break;
                                }
                            }
                            break;
                        case '7':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '~':
                                    consumed = 4;
                                    return Event::kF6;
                                default:
                                    break;
                                }
                            }
                            break;
                        case '8':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '~':
                                    consumed = 4;
                                    return Event::kF7;
                                default:
                                    break;
                                }
                            }
                            break;
                        case '9':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '~':
                                    consumed = 4;
                                    return Event::kF8;
                                default:
                                    break;
                                }
                            }
                            break;
                        case ';':
                            if (size >= 4)
                            {
                                switch (data[3])
                                {
                                case '2':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case 'A':
                                            consumed = 5;
                                            return Event::kShiftArrowUp;
                                        case 'B':
                                            consumed = 5;
                                            return Event::kShiftArrowDown;
                                        case 'C':
                                            consumed = 5;
                                            return Event::kShiftArrowRight;
                                        case 'D':
                                            consumed = 5;
                                            return Event::kShiftArrowLeft;
                                        case 'F':
                                            consumed = 5;
                                            return Event::kShiftEnd;
                                        case 'H':
                                            consumed = 5;
                                            return Event::kShiftHome;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                case '1':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case 'A':
                                            consumed = 5;
                                            return Event::kAltArrowUp;
                                        case 'B':
                                            consumed = 5;
                                            return Event::kAltArrowDown;
                                        case 'C':
                                            consumed = 5;
                                            return Event::kAltArrowRight;
                                        case 'D':
                                            consumed = 5;
                                            return Event::kAltArrowLeft;
                                        case 'F':
                                            consumed = 5;
                                            return Event::kAltEnd;
                                        case 'H':
                                            consumed = 5;
                                            return Event::kAltHome;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                case '5':
                                    if (size >= 5)
                                    {
                                        switch (data[4])
                                        {
                                        case 'A':
                                            consumed = 5;
                                            return Event::kCtrlArrowUp;
                                        case 'B':
                                            consumed = 5;
                                            return Event::kCtrlArrowDown;
                                        case 'C':
                                            consumed = 5;
                                            return Event::kCtrlArrowRight;
                                        case 'D':
                                            consumed = 5;
                                            return Event::kCtrlArrowLeft;
                                        case 'F':
                                            consumed = 5;
                                            return Event::kCtrlEnd;
                                        case 'H':
                                            consumed = 5;
                                            return Event::kCtrlHome;
                                        default:
                                            break;
                                        }
                                    }
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case 'Z':
                    consumed = 2;
                    return Event::kShiftTab;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }
    return 0;
}
#endif
} // namespace termui