# decaf-compiler

## Description
This is a compiler project for the Decaf programming language. Decaf is a Java-like object-oriented programming language. The compiler uses Flex and Binson to do lexical and syntax analysis. After building an abstract syntax tree (AST), the compiler performs multiple passes of semantic analysis to support features of the Decaf language. Once all error checking is done, the compiler generates three-address code (TAC) as intermediate representation (IR), and emits MIPS assembly code. The MIPS assembly code can be executed with the SPIM simulator.

## Festures of the Decaf compiler
* Lexical and syntax analysis using Flex and Bison
* Semantic analysis based on Abstract syntax tree
* Three-address code intermediate representation
* MIPS as the target architecture

## References
* The Decaf Language
  > https://parasol.tamu.edu/courses/decaf/students/decafOverview.pdf
* CPTR 415 Compiler Construction - The Decaf Programming Language
  > http://computing.southern.edu/halterman/Courses/Spring2008/415/Assignments/decaf.html
* Abstract Syntax Tree (AST)
  > https://en.wikipedia.org/wiki/Abstract_syntax_tree
* Intermediate representation
  > https://en.wikipedia.org/wiki/Intermediate_representation
* Three-Address Code (TAC)
  > https://en.wikipedia.org/wiki/Three-address_code
* Flex
  > https://en.wikipedia.org/wiki/Flex_(lexical_analyser_generator)
* Bison
  > https://en.wikipedia.org/wiki/GNU_bison
* MIPS architecture
  > https://en.wikipedia.org/wiki/MIPS_architecture
* SPIM simulator - A MIPS processor similator
  > https://en.wikipedia.org/wiki/SPIM
