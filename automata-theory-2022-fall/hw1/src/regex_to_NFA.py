class RegexToNFAException(Exception):
    pass


def infix_to_postfix(infix: str):
    """
    Convert an infix form of regular expression into a corresponding postfix form.
    """
    operators = {".", "*", "+"}
    operands = {"0", "1", "e"}
    parenthesis_start = "("
    parenthesis_end = ")"
    stack: list[str] = []  # A stack to store the operators
    postfix = ""

    for c in infix:
        if c in operators:
            stack.append(c)
        elif c in operands:
            postfix += c
        elif c == parenthesis_start:
            stack.append(c)
        elif c == parenthesis_end:
            try:
                while True:
                    s = stack.pop()
                    if s == parenthesis_start:
                        break
                    postfix += s
            except IndexError:
                raise RegexToNFAException(
                    "Insufficient operators or starting parenthesis"
                )
        else:
            raise RegexToNFAException(f"Invalid character in the input regex: {c}")

    # Stack should be empty at the end of the interation.
    if len(stack) > 0:
        raise RegexToNFAException("Unused operators exist")

    return postfix


def make_NFA(table: list[set[int]], char: str):
    """
    Makes a simple NFA that accepts a single character.
    """
    q_offset = len(table) // 3
    q_ini, q_fin = q_offset, q_offset + 1

    # Extend the table for the new q_fin value.
    diff = 3 * (q_fin + 1) - len(table)
    table.extend([set() for _ in range(diff)])

    char_table = {"e": 0, "0": 1, "1": 2}
    table[3 * q_ini + char_table[char]].update({q_fin})

    return table, q_ini, q_fin


def operate_NFA(table: list[set[int]], operator: str, *operands: tuple[int, int]):
    """
    Operates on the intermediate NFAs to make another NFA.
    """
    q_offset = len(table) // 3
    if operator == "*":
        if len(operands) != 1:
            raise RegexToNFAException("Invalid number of operands")

        q_ini, q_fin = q_offset, q_offset + 1

        # Extend the table.
        diff = 3 * (q_fin + 1) - len(table)
        table.extend([set() for _ in range(diff)])

        table[3 * q_ini].update({operands[0][0], q_fin})
        table[3 * operands[0][1]].update({operands[0][0], q_fin})
    else:
        if len(operands) != 2:
            raise RegexToNFAException("Invalid number of operands")

        if operator == ".":
            q_ini, q_fin = operands[0][0], operands[1][1]
            table[3 * operands[0][1]].update({operands[1][0]})
        elif operator == "+":
            q_ini, q_fin = q_offset, q_offset + 1

            # Extend the table.
            diff = 3 * (q_fin + 1) - len(table)
            table.extend([set() for _ in range(diff)])

            table[3 * q_ini].update({operands[0][0], operands[1][0]})
            table[3 * operands[0][1]].update({q_fin})
            table[3 * operands[1][1]].update({q_fin})

    return table, q_ini, q_fin


def postfix_to_NFA(regex: str):
    """
    Convert an postfix form of regular expression into a corresponding NFA transition table.
    The output is (q_ini, q_fin, table) where q_ini and q_fin are initial state and final state respectively, and
    table is a list of set where index 3i+0, 3i+1, 3i+2 each corresponds to a transition from (i, e), (i, 0), (i, 1).
    """
    # Initialize NFA with empty set case.
    q_ini, q_fin = (0, 1)
    table: list[set[int]] = [set()] * 3 * 2

    stack: list[tuple[int, int]] = []  # A stack to store intermediate NFAs
    unary_operators = {"*"}
    binary_operators = {".", "+"}
    operands = {"0", "1", "e"}

    for c in regex:
        if c in unary_operators:
            try:
                table, q_ini, q_fin = operate_NFA(table, c, stack.pop())
                stack.append((q_ini, q_fin))
            except IndexError:
                raise RegexToNFAException("Insufficient intermadiate NFAs")
        elif c in binary_operators:
            try:
                right, left = stack.pop(), stack.pop()  # The order is important.
                table, q_ini, q_fin = operate_NFA(table, c, left, right)
                stack.append((q_ini, q_fin))
            except IndexError:
                raise RegexToNFAException("Insufficient intermadiate NFAs")
        elif c in operands:
            table, q_ini, q_fin = make_NFA(table, c)
            stack.append((q_ini, q_fin))
        else:
            raise RegexToNFAException(f"Invalid character in the input regex: {c}")

    # If valid, the length of stack should be 0 or 1. If the length is 0, the regex denotes an empty set.
    if len(stack) > 1:
        raise RegexToNFAException("Unused intermediate NFAs exist")

    return table, q_ini, q_fin


if __name__ == "__main__":
    # Get an input regex as infix form from the standard input.
    regex_infix = input().strip()

    # First step: convert the input regex to postfix form.
    regex_postfix = infix_to_postfix(regex_infix)

    # Second step: convert the postifx regex to NFA transition table.
    table, q_ini, q_fin = postfix_to_NFA(regex_postfix)

    # Get the nubmer of states in the table.
    n_q = len(table) // 3

    # Print the output.
    print(f"{n_q} {q_ini} {q_fin}")
    for t in table:
        n = len(t)
        s = " ".join(map(lambda x: str(x), t))
        print(f"{n} {s}")
