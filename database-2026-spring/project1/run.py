import sys
import os
from concurrent.futures import Future
from threading import Thread
from lark import Lark, UnexpectedInput


PROMPT = "DB_2020-17316> "
GRAMMAR_FILE = "grammar.lark"
DEBUG = True

parser: Lark|None = None
parser_future: Future[Lark] = Future()


def build_parser_concurrent():
    """
    Kick off parser construction in a background thread.
    """
    def task():
        try:
            filepath = os.path.join(os.path.dirname(__file__), GRAMMAR_FILE)
            with open(filepath, 'r') as f:
                result = Lark(f, debug=DEBUG, strict=DEBUG, parser='lalr')
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


def eval_seq(seq: str) -> bool:
    """
    Returns True if the program should terminate. If syntax error is encountered
    in the middle of sentences, the remainings are ignored.
    """
    ensure_parser()
    assert parser is not None
    try:
        prettystr = parser.parse(seq).pretty()
        sys.stdout.write(PROMPT + prettystr + '\n')
        sys.stdout.flush()
    except UnexpectedInput:
        sys.stdout.write(PROMPT + "Syntax error\n")
        sys.stdout.flush()
    return False


def interact():
    buf: list[str] = []
    while True:
        if sys.stdin.isatty() and len(buf) == 0:
            sys.stdout.write(PROMPT)
            sys.stdout.flush()

        line: str = sys.stdin.readline()
        if line == '': # EOF
            break
        parts = [p.strip() for p in line.rsplit(';', 1)]
        assert len(parts) == 1 or len(parts) == 2

        # Last semicolon (';') found, run the sql engine.
        if len(parts) == 2:
            buf.append(parts[0])
            seq = ' '.join(buf) + ';' # Should add ';' since rsplitting removed it.
            buf = []
            if eval_seq(seq):
                break

        # Store the remaining part (after last ';') to the buffer.
        if len(parts[-1]) > 0:
            buf.append(parts[-1])
    return



if __name__ == '__main__':
    build_parser_concurrent()
    interact()
    sys.exit(0)

