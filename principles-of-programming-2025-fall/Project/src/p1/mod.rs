//! DO NOT CHANGE THIS FILE.

pub mod eval;
pub mod expr;
pub mod value;

#[cfg(test)]
mod tests {
    use super::expr::Expr;
    #[test]
    fn eval_test1() {
        // ((3 + 5) / 2) = 4
        let expr1 = Expr::ediv(Expr::eadd(Expr::enumb(3), Expr::enumb(5)), Expr::enumb(2));
        expr1.test_eval(4);

        // letfun (f x := (x - 1)) in
        // let n := f(42) in
        // let m := (n * 2) in
        // f(m)
        // = 81
        let expr3 = Expr::eletrec(
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
        expr3.test_eval(81);
    }

    #[test]
    fn eval_test2() {
        // letfun (f x := (x - 1)) in
        // let n := f(42) in
        // let m := (n * 2) in
        // f(m)
        // = 81
        let expr3 = Expr::eletrec(
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
        expr3.test_eval(81);
    }

    #[test]
    fn eval_test3() {
        // letfun (is_even x := if0 x then 0 else is_odd(x-1) ),
        //      (is_odd x := if0 x then 1 else is_even(x-1) ) in
        // is_even(7)
        // = 1
        fn expr_is_even_odd(n: i64) -> Expr {
            Expr::eletrec(
                vec![
                    Expr::efun(
                        "is_even",
                        "x",
                        Expr::eif0(
                            Expr::ename("x"),
                            Expr::enumb(0),
                            Expr::eapp(
                                Expr::ename("is_odd"),
                                Expr::esub(Expr::ename("x"), Expr::enumb(1)),
                            ),
                        ),
                    ),
                    Expr::efun(
                        "is_odd",
                        "x",
                        Expr::eif0(
                            Expr::ename("x"),
                            Expr::enumb(1),
                            Expr::eapp(
                                Expr::ename("is_even"),
                                Expr::esub(Expr::ename("x"), Expr::enumb(1)),
                            ),
                        ),
                    ),
                ],
                Expr::eapp(Expr::ename("is_even"), Expr::enumb(n)),
            )
        }

        expr_is_even_odd(7).test_eval(1);
        expr_is_even_odd(6).test_eval(0);
    }
}
