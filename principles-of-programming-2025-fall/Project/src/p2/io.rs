use std::io::{stdin, stdout, Stdin, Stdout, Write};

pub trait IOHandle {
    fn read_line(&self, buf: &mut String);
    fn print(&mut self, buf: &str);
}

pub struct StdIO {
    reader: Stdin,
}

impl StdIO {
    pub fn new() -> Self {
        Self { reader: stdin() }
    }
}

impl IOHandle for StdIO {
    fn read_line(&self, buf: &mut String) {
        self.reader.read_line(buf).unwrap();
    }

    fn print(&mut self, buf: &str) {
        println!("Print by StdIO! : {}", buf);
    }
}

pub struct FixedInputIO {
    input: i64,
}

impl FixedInputIO {
    pub fn new(input: i64) -> Self {
        Self { input }
    }
}

impl IOHandle for FixedInputIO {
    fn read_line(&self, buf: &mut String) {
        *buf = format!("{}", self.input);
    }

    fn print(&mut self, buf: &str) {
        println!("Print by Fixed Input IO! : {}", buf);
    }
}
