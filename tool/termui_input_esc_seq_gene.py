#! /usr/bin/python3

"""Generate parser for escape sequences used for keyboard input."""

import collections
import re
import textwrap

# definitions of key + associated escape sequence
KeysDefinition = """\
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
"""

Indent = " " * 4


def generate_inner_code(graph, depth):
    """Generate code for parser at one level of the graph."""
    print(
        textwrap.indent(
            f"""\
if (size >= {depth+1})
{{
{Indent}switch(data[{depth}])
{Indent}{{""",
            prefix=Indent * (2 * depth + 1),
        )
    )

    for key, value in graph.items():
        print(textwrap.indent(f"case '{key}':", prefix=Indent * (2 * depth + 2)))
        if isinstance(value, dict):
            generate_inner_code(value, depth + 1)
            print(textwrap.indent("break;", prefix=Indent * (2 * depth + 3)))
        else:
            print(
                textwrap.indent(
                    f"consumed = {depth+1};\nreturn {value};",
                    prefix=Indent * (2 * depth + 3),
                )
            )

    print(
        textwrap.indent(
            f"""\
{Indent}default:
{Indent*2}break;
{Indent}}}
}}""",
            prefix=Indent * (2 * depth + 1),
        )
    )


def generate_parser_code(graph):
    """Generate code for parser."""
    print(
        """\
char32_t identify_esc_seq(const char *data, size_t size, size_t &consumed)
{"""
    )

    generate_inner_code(graph, 0)

    print(
        f"""\
{Indent}return 0;
}}"""
    )


def main():
    """Main function."""
    # parse keys definition
    KeyDef = collections.namedtuple("KeyDef", "seq event")
    keys = []
    for line in KeysDefinition.splitlines():
        if line:
            match = re.match(r'\s*\{(.*), "(.*)"\},', line)
            keys.append(KeyDef(match.group(2), match.group(1)))

    # generate graph
    graph = {}
    for key in keys:
        walk = graph
        for char in key.seq[:-1]:
            walk = walk.setdefault(char, {})
        walk[key.seq[-1]] = key.event

    # generate parser code
    generate_parser_code(graph)


if __name__ == "__main__":
    main()
