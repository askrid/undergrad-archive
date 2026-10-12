//! DO NOT CHANGE THIS FILE.

// Used in eval.rs: Version without memoization.

use std::{
    rc::Rc,
    sync::atomic::{AtomicI64, Ordering},
};

use crate::{common::linkedlist::List, p1::expr::FunDef};

/// Values: either a number (i64) or a closure (a function definition along with its environment).
#[derive(Debug, PartialEq, Clone)]
pub enum Val<'a> {
    VNum(i64),
    VClo(&'a FunDef, Rc<Env<'a>>),
}

/// Formatting utility for displaying the Val type
impl std::fmt::Display for Val<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Val::VNum(n) => write!(f, "{}", n),
            Val::VClo(fdef, _) => write!(f, "efun ({}) {}", fdef.0, fdef.1),
        }
    }
}

// static(global) values for eval.rs, only for memory leak checks.
// You cannot change these values explicitly.

/// The number of created enviroments
pub static ENV_NUM: AtomicI64 = AtomicI64::new(0);
/// The number of dropped enviroments
pub static DROP_NUM: AtomicI64 = AtomicI64::new(0);

/// Entries in the Env
#[derive(Debug, PartialEq, Clone)]
pub enum EnvEntry<'a> {
    EEVal(&'static str, Val<'a>),
    EEFuns(List<(&'static str, &'a FunDef)>),
}

/// Environment
#[derive(PartialEq, Debug)]
pub struct Env<'a> {
    pub parent: Option<Rc<Env<'a>>>,
    pub entry: EnvEntry<'a>,
    /// an ID only for debugging and memory leak checking.
    id: i64,
}
impl<'a> Env<'a> {
    pub fn new(parent: Option<Rc<Env<'a>>>, entry: EnvEntry<'a>) -> Env<'a> {
        let id = ENV_NUM.fetch_add(1, Ordering::SeqCst);
        Env { id, parent, entry }
    }
}
impl Drop for Env<'_> {
    fn drop(&mut self) {
        DROP_NUM.fetch_add(1, Ordering::SeqCst);
        self.parent = None
    }
}
