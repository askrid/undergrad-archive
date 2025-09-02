class CircularQueue():
    def __init__(self, maxSize):
        self.M = maxSize
        self.front = 0
        self.rear = 0
        self.queue = [None] * maxSize

    def enqueue(self, element):
        if not self.is_full() and type(element) == Person:
            self.rear = (self.rear+1) % self.M
            self.queue[self.rear] = element
            
            if self.is_full():
                print("Queue is full!")

        elif type(element) != Person:
            print("Element is not a Person class element")

    def dequeue(self):
        if not self.is_empty():
            self.front = (self.front+1) % self.M

            if self.is_empty():
                print("Queue is empty")
            
            return self.queue[self.front]
    
    def multi_dequeue(self, count):
        return [self.dequeue() for i in range(count)]

    def peek(self):
        if not self.is_empty():
            return self.queue[self.front+1]

    def is_empty(self):
        return self.front == self.rear

    def is_full(self):
        return self.front == (self.rear+1) % self.M


class Person():
    def __init__(self, name, age):
        self.name = name
        self.age = age

    def __add__(self, other):
        return self.age + other.age

    def __str__(self):
        return self.name

    def __gt__(self, other):
        return self.age > other.age

    def __lt__(self, other):
        return self.age < other.age

    def __repr__(self):
        return 'Person(name: %s, age: %d)' %(self.name, self.age)


# def main():

#     cg = CircularQueue(5)
#     cg.enqueue(Person('Apple', 24))
#     cg.enqueue(Person('Banna', 29))
#     cg.enqueue(Person('Cutie', 21))
#     cg.enqueue(Person('Elf', 26))
#     cg.enqueue('asdf')

#     person1 = cg.dequeue()
#     person2 = cg.dequeue()
#     peek1 = cg.peek()
#     person_list = cg.multi_dequeue(2)

#     print(person1, person2, peek1)
#     print(person_list)
#     print(person1 + person2)
#     print(person1 > person2, person1 < person2)
#     print(cg.front, cg.rear)

# main()