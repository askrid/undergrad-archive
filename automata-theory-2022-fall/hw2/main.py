from collections import deque


class DPAStack:
    """
    Stack abstraction for DPA.
    """

    def __init__(self, *args, **kwargs):
        self.container = deque(*args, **kwargs)

    def pop(self):
        return self.container.popleft()

    def push(self, element):
        return self.container.appendleft(element)

    def is_empty(self):
        return len(self.container) == 0

    def __repr__(self):
        return self.iter_to_str(self.container)

    @staticmethod
    def iter_to_str(obj):
        """
        Convert the iterable object to a string.
        """
        return "".join(map(lambda x: str(x), obj))


class DPA:
    parse_table = {
        "id": {
            "E": ["T", "E'"],
            "T": ["F", "T'"],
            "F": ["A"],
            "A": ["id"],
        },
        "(": {
            "E": ["T", "E'"],
            "T": ["F", "T'"],
            "F": ["(", "E", ")"],
        },
        ")": {
            "E'": [],
            "T'": [],
        },
        "+": {
            "E'": ["+", "T", "E'"],
            "T'": [],
        },
        "-": {
            "E'": ["-", "T", "E'"],
            "T'": [],
        },
        "*": {
            "T'": ["*", "F", "T'"],
        },
        "/": {
            "T'": ["/", "F", "T'"],
        },
        "#": {
            "E'": [],
            "T'": [],
        },
    }

    def __init__(self, in_str):
        self.in_str = in_str + "#"
        self.curr_alpha = ""

        # States = { p, q, qx, q#, r }
        self.state = "p"
        self.stack = DPAStack()
        self.parsed = ""

        self.halted = False

    def snapshot(self):
        return self.parsed + str(self.stack)

    def is_halted(self):
        return self.halted

    def is_accepted(self):
        return self.halted and self.state == "r"

    def run_step(self):
        """
        Run the DPA one step.
        """
        if self.halted:
            return

        elif self.state == "p":
            # At first, push "E" to the stack.
            if self.stack.is_empty():
                self.state = "q"
                self.stack.push("E")
            else:
                self.halted = True
                return

        elif self.state == "q":
            # The input string should have one or more alphabets including "#".
            self.curr_alpha = self.in_str[0]

            if self.curr_alpha == "#":
                # End of the input string.
                self.state = "q#"
            else:
                # Discard the first alphabet if the input is not empty.
                self.in_str = self.in_str[1:]
                self.state = "qx"

        elif self.state == "qx":
            # Halt if the stack is empty.
            if self.stack.is_empty():
                self.halted = True
                return

            alpha_resolved = self.resolve_alpha(self.curr_alpha)
            top = self.stack.pop()

            if top == self.curr_alpha:
                # Resolved alphabet was on the top of the stack.
                self.parsed += self.curr_alpha
                self.state = "q"
            else:
                # Stack variable was on the top of the stack.
                parse_options = self.parse_table.get(alpha_resolved)
                stack_push_list = parse_options.get(self.resolve_alpha(top))

                # Halt if no rule to apply.
                if stack_push_list is None:
                    self.halted = True
                    return

                # Push the stack variables in the proper order.
                for stack_var in reversed(stack_push_list):
                    if stack_var == "id":
                        # Stack should contain the original alphabet.
                        stack_var = self.curr_alpha
                    self.stack.push(stack_var)

        elif self.state == "q#":
            if self.stack.is_empty():
                self.state = "r"

            else:
                top = self.stack.pop()
                parse_options = self.parse_table.get("#")

                if parse_options.get(self.resolve_alpha(top)) is None:
                    self.halted = True
                    return

        elif self.state == "r":
            self.halted = True
            return

        return

    @staticmethod
    def resolve_alpha(alpha):
        """
        Return "id" if the alphabet is an operand.
        """
        if alpha in [
            "a",
            "b",
            "c",
            "d",
            "x",
            "y",
            "z",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
        ]:
            return "id"
        else:
            return alpha


if __name__ == "__main__":
    # Run topdown parsing.
    dpa = DPA(input())
    history = []

    while True:
        dpa.run_step()

        # Append DPA snapshot to the history only if it is different from the last one.
        if len(history) == 0 or history[-1] != dpa.snapshot():
            history.append(dpa.snapshot())

        if dpa.is_halted():
            break

    # The DPA accepted the input string.
    if dpa.is_accepted():
        # Assert len(parse_history) > 0
        print(history[0])
        for ph in history[1:]:
            print(f"=> {ph}")

    # The DPA rejected the input string.
    else:
        print("reject")
