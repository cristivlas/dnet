# dnet

A back-end for Walter Bright' s D language compiler, written in C++. Generates textual IL and invokes ILASM to produce .NET executable files. Uses the Digital Mars D compiler front-end and supports D version 2.0

This is a back-end (code generator) for the D 2.0 Programming Language Compiler.

It produces textual IL code and invokes the ILASM utility to turn it into executables, the same way as early C and C++ compilers generated ASM code and invoked a separate assembler program to produce obj files.

The source code for the project is located under dmd/backend.net, all other source code (front-end, D libraries, etc) is not part of the project, and it is provided only to give the complete context to the back-end (and to allow people to download and build the project as painlessly as possible -- it does not make sense to download the front-end from one site and the back-end from another, before one can piece them together).

The back-end code is published under the Microsoft Public License.

The back-end code is not of production quality, it is intended for research and educational purposes. The D Programming Language is a fairly complex language, and non-trivial features such as TLS and closures make it an interesting case study for generating IL code.

The projects aims to demonstrate the feasibility (and benefits) of interoperability between D and the .NET platform (and family of languages).
