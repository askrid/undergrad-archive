import sys
import os
from concurrent.futures import Future
from threading import Thread
from typing import cast
from lark import Lark, UnexpectedInput, Transformer


PROMPT = "DB_2020-17316> "
GRAMMAR_FILE = "grammar.lark"
DEBUG = False

parser: Lark|None = None
parser_future: Future[Lark] = Future()


class NoopTransformer(Transformer):
    """
    NoopTransformer reduces a subtree that represents a query into a descriptive
    text, then populate the result to the root. The result is either string, or
    None in case of EXIT command.
    """
    type Result = str|None

    def create_table_query(self, _) -> str:
        return "'CREATE TABLE' requested"

    def drop_table_query(self, _) -> str:
        return "'DROP TABLE' requested"

    def explain_query(self, _) -> str:
        return "'EXPLAIN' requested"

    def describe_query(self, _) -> str:
        return "'DESCRIBE' requested"

    def desc_query(self, _) -> str:
        return "'DESC' requested"

    def show_tables_query(self, _) -> str:
        return "'SHOW TABLES' requested"

    def rename_table_query(self, _) -> str:
        return "'RENAME TABLE' requested"

    def truncate_table_query(self, _) -> str:
        return "'TRUNCATE TABLE' requested"

    def select_query(self, _) -> str:
        return "'SELECT' requested"

    def insert_query(self, _) -> str:
        return "'INSERT' requested"

    def delete_query(self, _) -> str:
        return "'DELETE' requested"

    def update_query(self, _) -> str:
        return "'UPDATE' requested"

    def exit_query(self, _) -> None:
        return None

    def query(self, args: list[Result]) -> Result:
        assert len(args) == 1
        return args[0]

    def command(self, args: list[Result]) -> Result:
        assert len(args) == 1
        return args[0]


def build_parser_concurrent():
    """
    Kick off parser construction in a background thread.
    """
    def task():
        try:
            filepath = os.path.join(os.path.dirname(__file__), GRAMMAR_FILE)
            with open(filepath, 'r') as f:
                # Lexer MUST be basic to disallow keywords as identifiers.
                result = Lark(f, debug=DEBUG, strict=DEBUG, start="command",
                              lexer="basic", parser="lalr",
                              transformer=NoopTransformer())
            parser_future.set_result(result)
        except Exception as e:
            parser_future.set_exception(e)

    thread = Thread(target=task, daemon=True)
    thread.start()


def ensure_parser():
    """
    Block until the parser is ready and set the global parser.
    """
    global parser
    if parser is None:
        parser = parser_future.result()


def process_seq(*seq: str) -> bool:
    """
    Takes a sequence of sql statements and prints each result. Returns `True` if
    the process should terminate. If syntax error is encountered in the middle
    of the statements, the remainings are ignored.
    """
    ensure_parser()
    assert parser is not None
    for s in seq:
        try:
            res = cast(NoopTransformer.Result, parser.parse(s))
            if res is None:
                return True
            sys.stdout.write(PROMPT + res + '\n') 
        except UnexpectedInput:
            sys.stdout.write(PROMPT + "Syntax error\n")
            break # Stop after failure.
    sys.stdout.flush()
    return False


def interact():
    """
    Process I/O.
    """
    buf: list[str] = [] # Buffer to memorize input state.
    while True:
        if sys.stdin.isatty() and len(buf) == 0:
            sys.stdout.write(PROMPT)
            sys.stdout.flush()

        line: str = sys.stdin.readline()
        if line == '': # EOF
            break
        parts = [p.strip() for p in line.split(';')]
        assert len(parts) > 0

        # At least one semicolon (';') found, run the sql engine.
        if len(parts) > 1:
            # Build the first statement with the buffer.
            buf.append(parts[0])
            part0 = ' '.join(buf)
            buf = []

            if process_seq(part0, *parts[1:-1]):
                break

        # Store the part after the last semicolon to the buffer.
        if len(parts[-1]) > 0:
            buf.append(parts[-1])
    return


if __name__ == '__main__':
    build_parser_concurrent()
    interact()
    sys.exit(0)

