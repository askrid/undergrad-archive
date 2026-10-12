//! DO NOT CHANGE THIS FILE.

use std::fmt::Display;

use crate::common::linkedlist::List;

#[derive(Debug, Eq, Hash, PartialEq)]
pub enum IO {
    // Read a line text and bind to parameter's name given as first argument
    ReadLine(&'static str),
    // Print an expression
    Print(Box<Expr>),
}

impl IO {
    pub fn iread(x: &'static str) -> Self {
        Self::ReadLine(x)
    }
    pub fn iprint(e: Expr) -> Self {
        Self::Print(Box::new(e))
    }
}

// impl ToString for IO {
//     fn to_string(&self) -> String {
//         match self{
//             IO::ReadLine(x) => format!("{x} = read_line"),
//             IO::Print(expr) => expr.to_string(),
//         }
//     }
// }
pub type IOAction = (List<IO>, Box<Expr>);
/// Defines a function, specifying the input parameter’s name and the expression for the function body.
#[derive(Debug, Eq, Hash, PartialEq)]
pub enum FunDef {
    Fun(&'static str, Box<Expr>),
    IOFun(&'static str, IOAction),
}

/// Add new variant, IOAction.
#[derive(Debug, Eq, Hash, PartialEq)]
pub struct Bind {
    pub(super) binder: &'static str,
    pub(super) fd: FunDef,
}

/// The type of expressions, providing the abstract syntax for the language.
#[derive(Debug, Eq, Hash, PartialEq)]
pub enum Expr {
    ENum(i64),
    EAdd(Box<Expr>, Box<Expr>),
    ESub(Box<Expr>, Box<Expr>),
    EMul(Box<Expr>, Box<Expr>),
    EDiv(Box<Expr>, Box<Expr>),
    EIf0(Box<Expr>, Box<Expr>, Box<Expr>),
    EName(&'static str),
    EApp(Box<Expr>, Box<Expr>),
    ELetVal(&'static str, Box<Expr>, Box<Expr>),
    ELetFuns(List<Bind>, Box<Expr>),
    ERunIO(Box<Expr>),
}

use Expr::*;
/// Use these functions instead of directly calling the constructors of Expr.
/// Examples can be found in the tests.
impl Expr {
    pub fn enumb(n: i64) -> Expr {
        ENum(n)
    }
    pub fn eadd(e1: Expr, e2: Expr) -> Expr {
        EAdd(Box::new(e1), Box::new(e2))
    }
    pub fn esub(e1: Expr, e2: Expr) -> Expr {
        ESub(Box::new(e1), Box::new(e2))
    }
    pub fn emul(e1: Expr, e2: Expr) -> Expr {
        EMul(Box::new(e1), Box::new(e2))
    }
    pub fn ediv(e1: Expr, e2: Expr) -> Expr {
        EDiv(Box::new(e1), Box::new(e2))
    }
    pub fn eif0(c: Expr, e1: Expr, e2: Expr) -> Expr {
        EIf0(Box::new(c), Box::new(e1), Box::new(e2))
    }
    pub fn ename(x: &'static str) -> Expr {
        EName(x)
    }
    pub fn eapp(f: Expr, a: Expr) -> Expr {
        EApp(Box::new(f), Box::new(a))
    }
    pub fn eletval(x: &'static str, v: Expr, e: Expr) -> Expr {
        ELetVal(x, Box::new(v), Box::new(e))
    }
    pub fn eletrec(binds: Vec<Bind>, e: Expr) -> Expr {
        ELetFuns(List::from_vec(binds), Box::new(e))
    }
    pub fn efun(ename: &'static str, x: &'static str, e: Expr) -> Bind {
        Bind {
            binder: ename,
            fd: FunDef::Fun(x, Box::new(e)),
        }
    }
    pub fn eiofun(ename: &'static str, x: &'static str, action: IOAction) -> Bind {
        Bind {
            binder: ename,
            fd: FunDef::IOFun(x, action),
        }
    }
    pub fn elam(x: &'static str, e: Expr) -> Expr {
        Expr::eletrec(vec![Expr::efun("☆ ", x, e)], Expr::ename("☆ "))
    }
    pub fn erun(e: Expr) -> Expr {
        Expr::ERunIO(Box::new(e))
    }
}

// Formatting utility used for printing the Expr type.

impl Display for IO {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            IO::ReadLine(x) => write!(f, "readline {x}"),
            IO::Print(expr) => write!(f, "Print {expr}"),
        }
    }
}

impl Display for List<IO> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let ios = self
            .iter()
            .fold(String::new(), |acc, io| format!("{acc} ({io})"));
        write!(f, "({ios})")
    }
}

impl Display for FunDef {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            FunDef::Fun(farg, e) => {
                write!(f, "({farg}) => {e}")
            }
            FunDef::IOFun(farg, (iolist, e)) => {
                write!(f, "({farg}) => ({iolist}) e")
            }
        }
    }
}

impl Display for Expr {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ENum(n) => write!(f, "{}", n),
            EAdd(e1, e2) => write!(f, "({e1} + {e2})"),
            ESub(e1, e2) => write!(f, "({e1} - {e2})"),
            EMul(e1, e2) => write!(f, "({e1} * {e2})"),
            EDiv(e1, e2) => write!(f, "({e1} / {e2})"),
            EIf0(c, tr, fl) => write!(f, "if {c} = 0 then {tr} else {fl}"),
            EName(x) => write!(f, "{x}"),
            EApp(ef, earg) => write!(f, "{ef}({earg})"),
            ELetVal(ename, e1, e2) => {
                write!(f, "let {ename} := {e1} in {e2}")
            }
            ELetFuns(binds, e) => {
                let bs = binds.iter().fold(String::new(), |acc, x| {
                    let binder = x.binder;
                    let fundef = &x.fd;
                    format!("{acc} ({binder} := {fundef})")
                });
                write!(f, "letfun {}in {}", bs, e.to_string())
            }
            ERunIO(e) => {
                write!(f, "Run {e}")
            }
        }
    }
}
