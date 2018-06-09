Notes for the .NET Backend
===========================

Compiling the dnet assembly place-holder under Visual Studio Express 2008:
csc /t:library /out:dnetlib.dll /debug+ marz\runtime\runtime.cs


The compiler can be also built on Linux (although the .NET backend does not work very well on that platform as of now).

Compiling dnet under Linux:
cd dmd
make depend
make

Compiling the dnetlib assembly (runtime support) under linux:
gmcs -debug+ -t:library -out:dnetlib.dll marz/runtime/runtime.cs


