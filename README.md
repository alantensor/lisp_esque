# MyLisp: A Lightweight Lisp Dialect Compiler

MyLisp is a lightweight compiler for a custom Lisp dialect, written in ANSI C. It uses Flex for lexical analysis and Bison for parsing, drawing inspiration from the "Build Your Own Lisp" book by Daniel Holden.

## Features

*   Basic Lisp syntax (S-Expressions, Q-Expressions)
*   Numbers, Strings, Symbols
*   Arithmetic operations: `+`, `-`, `*`, `/`, `%`, `^`
*   List manipulation functions: `list`, `head`, `tail`, `join`, `cons`, `len`, `init`, `eval`
*   Variable definition and assignment: `def`, `=`
*   User-defined functions (lambdas): `\\` (or `lambda`)
*   Conditional execution: `if`
*   Comparison operators: `>`, `<`, `>=`, `<=`, `==`, `!=`
*   File loading: `load "filename.mylisp"`
*   Printing to console: `print`
*   Error handling: `error "message"`
*   Interactive Read-Eval-Print Loop (REPL)
*   Ability to execute Lisp files directly

## Building the Compiler

To build the compiler, you need `flex`, `bison`, and a C compiler (like `gcc` or `clang`).
Navigate to the project's root directory and run:

```bash
make
```

This will generate the `mylisp` executable in the project root.

## Running MyLisp

### Interactive REPL

To start the REPL, simply run the compiled executable:

```bash
./mylisp
```

You can then type Lisp expressions directly into the prompt:

```
mylisp> (+ 2 3)
5
mylisp> (def {x} 10)
()
mylisp> x
10
mylisp> (if (> x 5) {print "Greater"} {print "Smaller or Equal"})
"Greater"
()
mylisp> (load "examples/hello.mylisp")
... output of hello.mylisp ...
mylisp> quit
```

Type `quit` to exit the REPL.

### Executing Files

You can also execute a `.mylisp` file directly by passing it as a command-line argument:

```bash
./mylisp your_file.mylisp
```

Or multiple files:

```bash
./mylisp file1.mylisp file2.mylisp
```

## Project Structure

*   `Makefile`: Defines build rules.
*   `src/`: Contains all source code.
    *   `common.h`: Common headers and forward declarations.
    *   `types.h`, `types.c`: Lisp data type definitions (lval, lenv) and management functions.
    *   `lexer.l`: Flex definitions for tokenizing input.
    *   `parser.y`: Bison grammar for parsing Lisp expressions and building an AST.
    *   `eval.h`, `eval.c`: Lisp expression evaluation logic and built-in functions.
    *   `main.c`: Main program entry point, REPL, and file processing logic.

## Inspiration

This project is heavily inspired by Daniel Holden's excellent book, [Build Your Own Lisp](http://www.buildyourownlisp.com/).

## Future Development

*   Macros
*   More advanced error reporting and recovery
*   Garbage collection
*   Further standard library functions
*   Optimizations
