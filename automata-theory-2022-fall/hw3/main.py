from enum import Enum


class UTMException(Exception):
    pass


class D(Enum):
    LEFT = "L"
    RIGHT = "R"
    STATIONARY = "S"


# Transition table type
# (before_state, read_letter): (after_state, write_letter, direction)
Trans = dict[tuple[int, str], tuple[int, str, D]]


class Tape:
    """
    Tape to be used in the Turing machines.
    """

    def __init__(self, s: str = ""):
        self.container = ["#"] + list(s)
        self.head = 0

    def read(self):
        return self.container[self.head]

    def write(self, write_letter: str, direction: D):
        """
        Write a letter on the tape and move the head to the given directon.
        """
        org_letter = self.container[self.head]
        self.container[self.head] = write_letter

        if direction == D.LEFT:
            if self.head == 0:
                self.container[self.head] = org_letter  # Restore the original letter.
                UTMException("Cannot move to left at the first cell of a tape.")

            self.head -= 1

        if direction == D.RIGHT:
            self.head += 1

            # Add '#' if the head extended the tape.
            if self.head == len(self.container):
                self.container.append("#")

        if direction == D.STATIONARY:
            pass  # explicit do-nothing

        # Check the invariant.
        assert self.head < len(self.container)
        return

    def __repr__(self):
        return "".join(self.container).strip("#")


class UTM:
    """
    Universal Turing Machine simulator
    """

    def run(self, trans: Trans, input_string: str):
        """
        Run the Turing machine with the given input.
        """
        trans = trans.copy()
        state = 0
        tape = Tape(input_string)

        while True:
            # Get 'todo' from the transition table.
            read_letter = tape.read()
            todo = trans.get((state, read_letter), None)

            # Halted, if not in the transition table.
            if todo is None:
                break

            # Write to tape and transfer state.
            (next_state, write_letter, direction) = todo
            tape.write(write_letter, direction)
            state = next_state

        return state, str(tape)


if __name__ == "__main__":
    # N is not essentially the number of states.
    line = input()
    N, tape_letters = line.split(" ")
    N = int(N)

    # Parse tape letters
    tape_letters = list(tape_letters)

    # Transition function as a table
    trans: Trans = {}
    for before_state in range(N):
        for read_letter in tape_letters:
            line = input()
            if not line:
                raise UTMException("Transition function is incomplete.")

            # Parse the line
            after_state, write_letter, direction = line.split(" ")
            after_state = int(after_state)
            direction = D(direction)

            # Update the transition table.
            trans[(before_state, read_letter)] = (after_state, write_letter, direction)

    # Get input strings.
    num_inputs = int(input())
    inputs: list[str] = []
    for _ in range(num_inputs):
        inputs.append(input().strip())

    # Run the Turing machine and print the results.
    tm = UTM()
    for input_string in inputs:
        state, tape = tm.run(trans, input_string)
        print(f"{tape} {state}")
