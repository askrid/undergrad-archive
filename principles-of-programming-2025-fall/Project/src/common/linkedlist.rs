//! DO NOT CHANGE THIS FILE.

/// Represents a list of type `T`, where each entry has full ownership.
#[derive(Debug, Eq, Hash, PartialEq, Clone, Default)]
pub enum List<T> {
    #[default]
    Nil,
    Cons(T, Box<List<T>>),
}

impl<T> List<T> {
    /// Creates an empty list.
    pub fn new() -> List<T> {
        Nil
    }

    /// Returns an iterator over the elements of the list.
    pub fn iter(&self) -> ListIterator<T> {
        self.into_iter()
    }

    /// Constructs a new list by prepending an element to an existing list.
    pub fn cons(hd: T, tl: List<T>) -> List<T> {
        Cons(hd, Box::new(tl))
    }

    /// Finds the first element in the list for which the given predicate returns `true`.
    pub fn find<F>(&self, check: F) -> Option<&T>
    where
        F: Fn(&T) -> bool,
    {
        match self {
            Nil => None,
            Cons(h, t) => {
                if check(h) {
                    Some(h)
                } else {
                    t.find(check)
                }
            }
        }
    }

    /// Constructs a list from a vector by iteratively adding its elements.
    pub fn from_vec(vec: Vec<T>) -> Self {
        let mut result = List::new();
        for x in vec.into_iter() {
            result = List::cons(x, result);
        }
        result
    }
}
use List::*;

/// Defines an iterator for the `List` type.
/// This allows iteration over a `List` using standard iterator methods.
/// Check https://doc.rust-lang.org/std/iter/trait.Iterator.html
pub struct ListIterator<'a, T> {
    current: &'a List<T>,
}

impl<'a, T> IntoIterator for &'a List<T> {
    type Item = &'a T;
    type IntoIter = ListIterator<'a, T>;

    fn into_iter(self) -> Self::IntoIter {
        ListIterator { current: self }
    }
}

impl<'a, T> Iterator for ListIterator<'a, T> {
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item> {
        match self.current {
            Nil => None,
            Cons(head, tail) => {
                self.current = tail;
                Some(head)
            }
        }
    }
}
