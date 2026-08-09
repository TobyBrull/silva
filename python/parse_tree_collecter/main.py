#!/usr/bin/env python3

"""
Collect every `silva::parse_tree_t::to_string()` from stdin and write them into files.
"""

import argparse
import dataclasses
import os
import subprocess
import sys

from rich.text import Text
from textual.app import App, ComposeResult
from textual.widgets import Footer, Header, OptionList, Static
from textual.widgets.option_list import Option


def is_node_line(line: str) -> bool:
    ls = line.strip()
    return ls.startswith("[") or ls.startswith("digraph ")


def is_never_node_line(line: str) -> bool:
    return len(line) == 0 or (
        len(line) <= 5 and line.startswith("  ") and line.endswith('"')
    )


@dataclasses.dataclass
class TreeInfo:
    index: int
    lines: list[str]

    def root_node(self) -> str:
        return self.lines[0][:20].strip()


def find_parse_trees(text: str) -> list[TreeInfo]:
    lines = text.splitlines()
    retval = []
    index = 0
    start = None
    for i, line in enumerate(lines):
        if line.startswith('  "'):
            line = line[3:]
        if start is not None:
            if is_never_node_line(line):
                retval.append(TreeInfo(index=index, lines=lines[start:i]))
                index += 1
                start = None
        else:
            if is_node_line(line):
                start = i

    return retval


@dataclasses.dataclass
class Command:
    cmd: list[str]
    description: str

    def __str__(self) -> str:
        return f"{' '.join(self.cmd):80} # {self.description}"

    def run(self) -> str:
        try:
            returncode = subprocess.run(self.cmd).returncode
        except OSError as e:
            return f"could not run {self.cmd[0]}: {e}"
        return f"exit code {returncode}: {' '.join(self.cmd)}"


def reopen_stdin_from_tty() -> None:
    """
    This script reads its input from stdin exhaustively. Point it at the terminal again, so
    that the TUI (and any command we spawn) can read from it.
    """
    fd = os.open("/dev/tty", os.O_RDONLY)
    os.dup2(fd, sys.stdin.fileno())
    os.close(fd)
    sys.stdin = open(sys.stdin.fileno(), closefd=False)


class CommandsApp(App[None]):
    TITLE = "parse-tree collecter"
    CSS = """
    OptionList { height: 1fr; border: none; }
    #status { padding: 0 1; color: $text-muted; }
    """
    BINDINGS = [
        ("q", "quit", "Quit"),
        ("escape", "quit", "Quit"),
        ("j", "cursor_down", "Down"),
        ("k", "cursor_up", "Up"),
    ]

    def __init__(self, commands: list[Command]) -> None:
        super().__init__()
        self.commands = commands
        self.sub_title = f"{len(commands)} commands"
        reopen_stdin_from_tty()

    def prompt(self, command: Command) -> Text:
        retval = Text(" ".join(command.cmd), style="bold")
        if command.description:
            retval.append(f"  # {command.description}", style="dim")
        return retval

    def compose(self) -> ComposeResult:
        yield Header()
        yield OptionList(*(Option(self.prompt(command)) for command in self.commands))
        yield Static("hit enter to run the selected command", id="status")
        yield Footer()

    def on_mount(self) -> None:
        self.query_one(OptionList).focus()

    def action_cursor_down(self) -> None:
        self.query_one(OptionList).action_cursor_down()

    def action_cursor_up(self) -> None:
        self.query_one(OptionList).action_cursor_up()

    def on_option_list_option_selected(self, event: OptionList.OptionSelected) -> None:
        command = self.commands[event.option_index]
        with self.suspend():
            status = command.run()
        self.query_one("#status", Static).update(status)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--output-pattern", default="var/parse_tree_{I}.txt")
    parser.add_argument("-p", "--pager", default="nvim")
    parser.add_argument("-d", "--diff-tool", default="nvim -d")
    parser.add_argument("--tui", action="store_true")
    args = parser.parse_args()

    text: str = sys.stdin.read()
    trees = find_parse_trees(text)

    if not trees:
        print("no parse-trees found in output", file=sys.stderr)
        return 1

    commands = []

    filenames = []
    for tree in trees:
        filename = args.output_pattern.format(I=tree.index)
        with open(filename, "w") as f:
            f.write("\n".join(tree.lines))
            f.write("\n")
        filenames.append(filename)
        commands.append(
            Command(
                cmd=[args.pager, filename],
                description=f"{len(tree.lines)} lines: {tree.root_node()}",
            )
        )

    for i in range(0, len(filenames), 2):
        commands.append(
            Command(
                cmd=args.diff_tool.split() + filenames[i : i + 2],
                description="",
            )
        )

    if args.tui:
        CommandsApp(commands).run()
    else:
        for command in commands:
            print(command)


if __name__ == "__main__":
    sys.exit(main())
