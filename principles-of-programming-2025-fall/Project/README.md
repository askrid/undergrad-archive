# Principles of Programming Assignment - Project

## Objective

Develop an interpreter for a toy language.
- Replace `todo!()` in `p1/eval.rs` with appropriate Rust code to implement the evaluation function

## Requirements
Your interpreter must adhere to the semantics of the toy language described in `project.pdf`.

1. **Safe Rust**:
   - Use only safe Rust. Any use of `unsafe` code will result in rejection.
2. **Library Restrictions**:
   - You may only use the following standard library data types and methods:
     - [`std::rc::Rc`](https://doc.rust-lang.org/std/rc/struct.Rc.html)
     - [`std::cell::RefCell`](https://doc.rust-lang.org/std/cell/struct.RefCell.html)
     - [`std::collections::HashMap`](https://doc.rust-lang.org/std/collections/struct.HashMap.html)
     - [`std::option::Option`](https://doc.rust-lang.org/std/option/enum.Option.html)
     - [`std::string::String`](https://doc.rust-lang.org/std/string/struct.String.html)
     - Methods from the [`std::iter::Iterator`](https://doc.rust-lang.org/std/iter/trait.Iterator.html) trait.

### Examples of Allowed Methods:
- `std::rc::Rc::{new, clone, as_ref, deref}`
- `std::cell::RefCell::{new, borrow, borrow_mut}`
- `std::collections::HashMap::{new, get, insert}`
- `std::option::Option::{is_some, is_none, map, map_or_else, unwrap, unwrap_or_else}`
- `std::iter::fold`
- [`std::panic!`](https://doc.rust-lang.org/std/macro.panic.html)


## Grading Considerations

## Further Notes
- Currently, the testing function for problem 1 is not implemented yet. We will update it as soon as possible. 
- Also, skeleton code for problem 2 will be revealed soon.

Timeout: 30 sec.

## How to Test Your Code
You can check whether your code is succesfully compiled by following commands.
```bash
cargo check
```

Also, you can run the basic test cases by followings. 

```bash
cargo test
```



## How to Format and Lint Your Code
To automatically format your code:
```bash
cargo fmt
```

To analyze your code and receive suggestions for fixes:
```bash
cargo clippy
```

## Submission Guidelines
- Only the provided `p1/eval.rs` files in the `src` directory will be used for testing. **Any additional files you create will not be included.**
- Submissions will be graded based on the latest commit to the main branch befor the deadline. Ensure your changes are committed and pushed to the repository.

```bash
git add src
git commit -m "Add a meaningful commit message"
git push
```

## Due date
- Deadline: 2025/12/25 23:59:59 KST

## Errata and Ambiguities
- A protected `errata` branch will contain any corrections or updates. Check this branch periodically for changes.
- If you ecounter ambiguities in the assignment description, report them via the GitHub issue tracker: https://github.com/snu-sf-class/pp202502/issues


