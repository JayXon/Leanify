@Echo off
Pushd %~dp0
SetLocal EnableDelayedExpansion
Set Args=--std=c++0x -O3 -msse2 -mfpmath=sse -flto -s -o "Leanify" Leanify.res 
For /r "%~dp0" %%i In (*.cpp *.c *.cc) Do (Set t=%%i && Set Args=!Args!!t:%~dp0=!)
For %%i In (lib\mozjpeg\jdcol565.c lib\mozjpeg\jdcolext.c lib\mozjpeg\jdmrg565.c lib\mozjpeg\jdmrgext.c lib\mozjpeg\jstdhuff.c) Do Set Args=!Args:%%i =!
windres --output-format=coff Leanify.rc Leanify.res
g++ %Args%
Del Leanify.res
Pause