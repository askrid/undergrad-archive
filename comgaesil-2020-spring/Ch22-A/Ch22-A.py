from queue import LifoQueue

class Tower():
    def __init__(self, *disks):
        self.disks = LifoQueue()
        for i in sorted(disks)[::-1]:
            self.disks.put(2*i - 1)

        self.width = self.disks.queue[0] if not self.disks.empty() else 0
        self.height = self.disks.qsize()

    def move_disc(self, other):
        disk = self.disks.get()
        other.disks.put(disk)

        if self.disks.empty(): self.width = 0
        self.height -= 1
        if other.disks.qsize() == 1: other.width = disk
        other.height += 1

    @classmethod
    def display_three(cls, tw1, tw2, tw3):
        display_width = max(tw1.width, tw2.width, tw3.width)
        display_height = max(tw1.height, tw2.height, tw3.height)

        print()
        for i in range(display_height-1, -1, -1):
            display_tw1 = ('*'*(tw1.disks.queue[i])).center(display_width) if i <= tw1.height - 1 else ' '*display_width
            display_tw2 = ('*'*(tw2.disks.queue[i])).center(display_width) if i <= tw2.height - 1 else ' '*display_width
            display_tw3 = ('*'*(tw3.disks.queue[i])).center(display_width) if i <= tw3.height - 1 else ' '*display_width
            print(display_tw1, display_tw2, display_tw3)
        print('-'*display_width, '-'*display_width, '-'*display_width)


def tower_of_hanoi(n):
    tower1 = Tower(*range(1, n+1))
    tower2 = Tower()
    tower3 = Tower()

    def move(n, tw1, tw2, tw3):
        if n >= 1:
            move(n-1, tw1, tw3, tw2)
            tw1.move_disc(tw3)
            Tower.display_three(tower1, tower2, tower3)
            move(n-1, tw2, tw1, tw3)

    Tower.display_three(tower1, tower2, tower3)
    move(n, tower1, tower2, tower3)


tower_of_hanoi(10)