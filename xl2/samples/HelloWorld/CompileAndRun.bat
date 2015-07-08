@rem The following files are necessary and have to be in the current directory for the moment :-(
@copy ..\..\xl.syntax .
@copy ..\..\native\xl.bytecode .

@rem Bootsrap version can't grok Hello World
@rem ..\..\bootstrap\xl2 HelloWorld.xl > HelloWorld_bootstrap.cpp
@rem cl /EHsc /I..\..  HelloWorld_bootstrap.cpp
@rem HelloWorld_bootstrap

@rem One day, the following statement will compile :-)
@rem ..\..\native\xl3 HelloWorld.xl > HelloWorld.cpp
@rem cl /EHsc /I..\..  HelloWorld.cpp
@rem HelloWorld

@rem Remark the -bootstrap option
..\..\native\xl3 -bootstrap HelloWorld.xl > HelloWorld_native.cpp
cl /EHsc /I..\..  HelloWorld_native.cpp
HelloWorld_native


