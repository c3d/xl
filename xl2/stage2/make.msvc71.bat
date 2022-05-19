@echo.
@echo Simple but effective batch to run the basic Native test under Windows.
@echo To use, run it in the command line with the build config name as the argument.
@echo After it runs, compare xl2.cpp and xl3.cpp. They must be identical.
@echo VC 7.1 (.NET 2003) must be ready for command line use (run its vcvars32.bat).
@echo If you got any questions about this batch, email [jcab.XL@JCABs-Rumblings.com].
@echo.

@if not "%1"=="" set _BUILD=%1
@if "%1"=="" set _BUILD=Debug

copy ..\xl.syntax .

..\bootstrap\xl compiler.xl > xl2.cpp
@if errorlevel 1 goto error
cl /GR /EHsc /I .. xl2.cpp
@if errorlevel 1 goto error

..\native\xl2 -bootstrap compiler.xl > xl3.cpp
@if errorlevel 1 goto error
cl /GR /EHsc /I .. xl3.cpp
@if errorlevel 1 goto error

..\native\xl3 -bootstrap compiler.xl > xl4.cpp
@if errorlevel 1 goto error
cl /GR /EHsc /I .. xl4.cpp
@if errorlevel 1 goto error

@goto done

:error

@echo.
@echo An error happened!!
@echo.

:done
