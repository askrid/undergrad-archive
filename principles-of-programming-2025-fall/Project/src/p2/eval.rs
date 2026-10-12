use std::rc::Rc;

use crate::{
    common::linkedlist::List,
    p2::{
        expr::{Bind, Expr, FunDef, IO},
        io::{FixedInputIO, IOHandle, StdIO},
        value::{Env, EnvEntry, Val},
    },
};

impl Expr {
    fn eval_arith<'a, 'b, H: IOHandle>(
        op: fn(i64, i64) -> i64,
        e1: &'a Expr,
        e2: &'a Expr,
        env_rc: Rc<Env<'a>>,
        h: &'b mut H,
    ) -> Val<'a> {
        use Val::*;
        let v1 = e1.eval_env(env_rc.clone(), h);
        let v2 = e2.eval_env(env_rc, h);
        match (v1, v2) {
            (VNum(n1), VNum(n2)) => VNum(op(n1, n2)),
            _ => panic!("Type error: arithmetic on non-numbers"),
        }
    }

    fn eval_if<'a, 'b, H: IOHandle>(
        e_cond: &'a Expr,
        e_then: &'a Expr,
        e_else: &'a Expr,
        env_rc: Rc<Env<'a>>,
        h: &'b mut H,
    ) -> Val<'a> {
        use Val::*;
        match e_cond.eval_env(env_rc.clone(), h) {
            VNum(0) => e_then.eval_env(env_rc, h),
            VNum(_) => e_else.eval_env(env_rc, h),
            _ => panic!("Type error: if0 condition must be a number"),
        }
    }

    fn eval_name<'a>(x: &'static str, env_rc: Rc<Env<'a>>) -> Val<'a> {
        use EnvEntry::*;
        use Val::*;
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

    fn eval_app<'a, 'b, H: IOHandle>(
        f: &'a Expr,
        arg: &'a Expr,
        env_rc: Rc<Env<'a>>,
        h: &'b mut H,
    ) -> Val<'a> {
        use EnvEntry::*;
        use Val::*;
        let closure = f.eval_env(env_rc.clone(), h);
        match closure {
            VClo(fundef, closure_env) => {
                let arg_val = arg.eval_env(env_rc, h);
                match fundef {
                    // Regular function: evaluate body immediately
                    FunDef::Fun(param, body) => {
                        let new_env = Rc::new(Env::new(Some(closure_env), EEVal(param, arg_val)));
                        body.eval_env(new_env, h)
                    }
                    // IO function: return thunk, DON'T execute IO yet
                    FunDef::IOFun(param, io_action) => {
                        let new_env = Rc::new(Env::new(Some(closure_env), EEVal(param, arg_val)));
                        VAct(io_action, new_env)
                    }
                }
            }
            _ => panic!("Type error: cannot call non-function"),
        }
    }

    fn eval_letval<'a, 'b, H: IOHandle>(
        x: &'static str,
        e1: &'a Expr,
        e2: &'a Expr,
        env_rc: Rc<Env<'a>>,
        h: &'b mut H,
    ) -> Val<'a> {
        use EnvEntry::*;
        let val = e1.eval_env(env_rc.clone(), h);
        let new_env = Rc::new(Env::new(Some(env_rc), EEVal(x, val)));
        e2.eval_env(new_env, h)
    }

    fn eval_letfuns<'a, 'b, H: IOHandle>(
        binds: &'a List<Bind>,
        e: &'a Expr,
        env_rc: Rc<Env<'a>>,
        h: &'b mut H,
    ) -> Val<'a> {
        use EnvEntry::*;
        let mut fun_list = List::new();
        for bind in binds.iter() {
            fun_list = List::cons((bind.binder, &bind.fd), fun_list);
        }
        let new_env = Rc::new(Env::new(Some(env_rc), EEFuns(fun_list)));
        e.eval_env(new_env, h)
    }

    fn eval_io<'a, 'b, H: IOHandle>(e: &'a Expr, env_rc: Rc<Env<'a>>, h: &'b mut H) -> Val<'a> {
        use EnvEntry::*;
        use Val::*;
        // First, evaluate e to get the IO action thunk
        let action = e.eval_env(env_rc, h);

        match action {
            VAct((io_list, final_expr), act_env) => {
                // Execute IO operations, building up environment
                let mut current_env = act_env;

                for io_op in io_list.iter() {
                    match io_op {
                        IO::ReadLine(name) => {
                            // Read from IOHandle
                            let mut buf = String::new();
                            h.read_line(&mut buf);
                            let num: i64 = buf.trim().parse().unwrap();
                            // Bind name to the read value
                            current_env =
                                Rc::new(Env::new(Some(current_env), EEVal(name, VNum(num))));
                        }
                        IO::Print(expr) => {
                            // Evaluate and print
                            let val = expr.eval_env(current_env.clone(), h);
                            h.print(&val.to_string());
                        }
                    }
                }

                // Evaluate the final expression in the extended environment
                final_expr.eval_env(current_env, h)
            }
            _ => panic!("Type error: runIO requires an IO action"),
        }
    }

    fn eval_env<'a, 'b, H: IOHandle>(&'a self, env_rc: Rc<Env<'a>>, h: &'b mut H) -> Val<'a> {
        use Expr::*;
        use Val::*;
        match self {
            ENum(n) => VNum(*n),
            EAdd(e1, e2) => Self::eval_arith(|x, y| x + y, e1, e2, env_rc, h),
            ESub(e1, e2) => Self::eval_arith(|x, y| x - y, e1, e2, env_rc, h),
            EMul(e1, e2) => Self::eval_arith(|x, y| x * y, e1, e2, env_rc, h),
            EDiv(e1, e2) => Self::eval_arith(|x, y| x / y, e1, e2, env_rc, h),
            EIf0(e_cond, e_then, e_else) => Self::eval_if(e_cond, e_then, e_else, env_rc, h),
            EName(x) => Self::eval_name(x, env_rc),
            EApp(f, arg) => Self::eval_app(f, arg, env_rc, h),
            ELetVal(x, e1, e2) => Self::eval_letval(x, e1, e2, env_rc, h),
            ELetFuns(binds, e) => Self::eval_letfuns(binds, e, env_rc, h),
            ERunIO(expr) => Self::eval_io(expr, env_rc, h),
        }
    }

    pub fn eval<H: IOHandle>(&self, mut h: H) -> Val<'_> {
        use EnvEntry::*;
        self.eval_env(Rc::new(Env::new(None, EEFuns(List::new()))), &mut h)
    }

    pub(crate) fn test_eval<H: IOHandle>(self, n: i64, h: H) {
        let r = self.eval(h);
        assert_eq!(r, Val::VNum(n))
    }
}

#[test]
fn eval_test1() {
    // ((3 + 5) / 2) = 4
    let h = FixedInputIO::new(3);
    let expr = Expr::ediv(Expr::eadd(Expr::enumb(3), Expr::enumb(5)), Expr::enumb(2));
    expr.test_eval(4, h);
}

fn eval_test2() {
    let h = FixedInputIO::new(3);
    // letfun (f x := (x - 1)) in
    // let n := f(42) in
    // let m := (n * 2) in
    // f(m)
    // = 81
    let expr = Expr::eletrec(
        vec![Expr::efun(
            "f",
            "x",
            Expr::esub(Expr::ename("x"), Expr::enumb(1)),
        )],
        Expr::eletval(
            "n",
            Expr::eapp(Expr::ename("f"), Expr::enumb(42)),
            Expr::eletval(
                "m",
                Expr::emul(Expr::ename("n"), Expr::enumb(2)),
                Expr::eapp(Expr::ename("f"), Expr::ename("m")),
            ),
        ),
    );
    expr.test_eval(81, h);
}

#[test]
fn eval_test3() {
    let h = FixedInputIO::new(3);

    // letfun (funIO f1 = (x) => (readline a) (a+x)) in
    // runIO (f1 3)
    // = 6
    let expr = Expr::eletrec(
        vec![Expr::eiofun(
            "f1",
            "x",
            (
                List::cons(IO::iread("a"), List::new()),
                Box::new(Expr::eadd(Expr::ename("a"), Expr::ename("x"))),
            ),
        )],
        Expr::erun(Expr::eapp(Expr::ename("f1"), Expr::enumb(3))),
    );
    expr.test_eval(6, h);
}

#[test]
#[should_panic]
fn eval_test4() {
    let h = FixedInputIO::new(3);

    // letfun (funIO f1 = (x) => (readline a) (a+x)) in
    // letfun (funIO f2 = (y) => (readline b) (runIO (f1 y)) + b ) in
    // (f2 10)
    // != 16
    let f1 = Expr::eiofun(
        "f1",
        "x",
        (
            List::cons(IO::iread("a"), List::new()),
            Box::new(Expr::eadd(Expr::ename("a"), Expr::ename("x"))),
        ),
    );
    let f2 = Expr::eiofun(
        "f2",
        "y",
        (
            List::cons(IO::iread("b"), List::new()),
            Box::new(Expr::eadd(
                Expr::erun(Expr::eapp(Expr::ename("f1"), Expr::ename("y"))),
                Expr::ename("b"),
            )),
        ),
    );

    let expr = Expr::eletrec(vec![f1, f2], Expr::eapp(Expr::ename("f2"), Expr::enumb(10)));
    expr.test_eval(16, h);
}

#[test]
fn eval_test5() {
    let h = FixedInputIO::new(3);

    // letfun (funIO f1 = (x) => (readline a) (a+x)) in
    // letfun (funIO f2 = (y) => (readline b) (runIO (f1 y)) + b ) in
    // runIO (f2 10)
    // = 16
    let f1 = Expr::eiofun(
        "f1",
        "x",
        (
            List::cons(IO::iread("a"), List::new()),
            Box::new(Expr::eadd(Expr::ename("a"), Expr::ename("x"))),
        ),
    );
    let f2 = Expr::eiofun(
        "f2",
        "y",
        (
            List::cons(IO::iread("b"), List::new()),
            Box::new(Expr::eadd(
                Expr::erun(Expr::eapp(Expr::ename("f1"), Expr::ename("y"))),
                Expr::ename("b"),
            )),
        ),
    );

    let expr = Expr::eletrec(
        vec![f1, f2],
        Expr::erun(Expr::eapp(Expr::ename("f2"), Expr::enumb(10))),
    );
    expr.test_eval(16, h);
}
