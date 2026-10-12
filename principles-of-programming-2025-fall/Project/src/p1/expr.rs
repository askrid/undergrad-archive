//! DO NOT CHANGE THIS FILE.

use crate::common::linkedlist::List;
use std::fmt::Write;
use Expr::*;

/// Defines a function, specifying the input parameter’s name and the expression for the function body.
pub type FunDef = (&'static str, Box<Expr>);

/// A tuple consisting of
/// !Deprecated in 2025! - a boolean (indicating whether the function is memoized; true means memoized, false means not memoized),
/// - a string (the function name),
/// - a function definition
pub type Bind = (&'static str, FunDef);

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
}

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
        (ename, (x, Box::new(e)))
    }
    pub fn elam(x: &'static str, e: Expr) -> Expr {
        Expr::eletrec(vec![Expr::efun("☆ ", x, e)], Expr::ename("☆ "))
    }
}

/// Formatting utility used for printing the Expr type.
impl std::fmt::Display for Expr {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ENum(n) => write!(f, "{}", n),
            EAdd(e1, e2) => write!(f, "({} + {})", e1, e2),
            ESub(e1, e2) => write!(f, "({} - {})", e1, e2),
            EMul(e1, e2) => write!(f, "({} * {})", e1, e2),
            EDiv(e1, e2) => write!(f, "({} / {})", e1, e2),
            EIf0(c, tr, fl) => {
                write!(f, "if {} = 0 then {} else {}", c, tr, fl)
            }
            EName(x) => write!(f, "{}", x),
            EApp(ef, earg) => {
                write!(f, "{}({})", ef, earg)
            }
            ELetVal(ename, e1, e2) => {
                write!(f, "let {} := {} in {}", ename, e1, e2)
            }
            ELetFuns(binds, e) => {
                let bs = binds.iter().fold(String::new(), |mut acc, x| {
                    write!(acc, "({} {} := {}) ", x.0, x.1 .0, x.1 .1).unwrap();
                    acc
                });
                write!(f, "letfun {}in {}", bs, e)
            }
        }
    }
}
