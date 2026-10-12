use std::{rc::Rc, sync::atomic::Ordering};

use crate::{
    common::linkedlist::List,
    p1::{
        expr::{Bind, Expr, FunDef},
        value::{
            Env,
            EnvEntry::*,
            Val::{self, *},
        },
    },
};

impl Expr {
    fn eval_arith<'a>(
        op: fn(i64, i64) -> i64,
        e1: &'a Expr,
        e2: &'a Expr,
        env_rc: Rc<Env<'a>>,
    ) -> Val<'a> {
        let v1 = e1.eval_env(env_rc.clone());
        let v2 = e2.eval_env(env_rc);
        match (v1, v2) {
            (VNum(n1), VNum(n2)) => VNum(op(n1, n2)),
            _ => panic!("Type error: arithmetic on non-numbers"),
        }
    }

    fn eval_if<'a>(
        e_cond: &'a Expr,
        e_then: &'a Expr,
        e_else: &'a Expr,
        env_rc: Rc<Env<'a>>,
    ) -> Val<'a> {
        match e_cond.eval_env(env_rc.clone()) {
            VNum(0) => e_then.eval_env(env_rc),
            VNum(_) => e_else.eval_env(env_rc),
            _ => panic!("Type error: if0 condition must be a number"),
        }
    }

    fn eval_name<'a>(x: &'static str, env_rc: Rc<Env<'a>>) -> Val<'a> {
        // Check if x is in the current environment entry
        match &env_rc.entry {
            EEVal(name, val) if *name == x => return val.clone(),
            EEFuns(binds) => {
                if let Some((_, fundef)) = binds.find(|(name, _)| *name == x) {
                    return VClo(fundef, env_rc.clone());
                }
            }
            _ => {}
        }
        // Not found in current entry, recurse to parent
        match &env_rc.parent {
            Some(parent) => Self::eval_name(x, parent.clone()),
            None => panic!("Unbound variable: {}", x),
        }
    }

    fn eval_app<'a>(f: &'a Expr, arg: &'a Expr, env_rc: Rc<Env<'a>>) -> Val<'a> {
        let closure = f.eval_env(env_rc.clone());
        match closure {
            VClo((param, body), closure_env) => {
                let arg_val = arg.eval_env(env_rc);
                let new_env = Rc::new(Env::new(Some(closure_env), EEVal(param, arg_val)));
                body.eval_env(new_env)
            }
            _ => panic!("Type error: cannot call non-function"),
        }
    }

    fn eval_letval<'a>(
        x: &'static str,
        e1: &'a Expr,
        e2: &'a Expr,
        env_rc: Rc<Env<'a>>,
    ) -> Val<'a> {
        let val = e1.eval_env(env_rc.clone());
        let new_env = Rc::new(Env::new(Some(env_rc), EEVal(x, val)));
        e2.eval_env(new_env)
    }

    fn eval_letfuns<'a>(binds: &'a List<Bind>, e: &'a Expr, env_rc: Rc<Env<'a>>) -> Val<'a> {
        let mut fun_list = List::new();
        for (name, fundef) in binds.iter() {
            fun_list = List::cons((*name, fundef), fun_list);
        }
        let new_env = Rc::new(Env::new(Some(env_rc), EEFuns(fun_list)));
        e.eval_env(new_env)
    }

    fn eval_env<'a>(&'a self, env_rc: Rc<Env<'a>>) -> Val<'a> {
        use crate::p1::expr::Expr::*;
        match self {
            ENum(n) => VNum(*n),
            EAdd(e1, e2) => Self::eval_arith(|x, y| x + y, e1, e2, env_rc),
            ESub(e1, e2) => Self::eval_arith(|x, y| x - y, e1, e2, env_rc),
            EMul(e1, e2) => Self::eval_arith(|x, y| x * y, e1, e2, env_rc),
            EDiv(e1, e2) => Self::eval_arith(|x, y| x / y, e1, e2, env_rc),
            EIf0(e_cond, e_then, e_else) => Self::eval_if(e_cond, e_then, e_else, env_rc),
            EName(x) => Self::eval_name(x, env_rc),
            EApp(f, arg) => Self::eval_app(f, arg, env_rc),
            ELetVal(x, e1, e2) => Self::eval_letval(x, e1, e2, env_rc),
            ELetFuns(binds, e) => Self::eval_letfuns(binds, e, env_rc),
        }
    }

    pub fn eval(&self) -> Val<'_> {
        self.eval_env(Rc::new(Env::new(None, EEFuns(List::new()))))
    }

    pub(crate) fn test_eval(self, n: i64) {
        let r = self.eval();
        assert_eq!(r, Val::VNum(n))
    }

    pub(crate) fn test_eval_leak_check(self, n: i64) {
        use super::value::{DROP_NUM, ENV_NUM};
        ENV_NUM.store(0, Ordering::SeqCst);
        DROP_NUM.store(0, Ordering::SeqCst);
        self.test_eval(n);
        assert_eq!(
            ENV_NUM.load(Ordering::SeqCst),
            DROP_NUM.load(Ordering::SeqCst)
        );
    }
}
