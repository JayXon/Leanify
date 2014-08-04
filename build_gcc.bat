@Echo off
Pushd %~dp0
SetLocal EnableDelayedExpansion
:: no -Wall here because miniz will generate a lot of warning
Set Args=--std=c++0x -Wno-multichar -O3 -o "Leanify" 
For /r "%~dp0" %%i In (*.cpp *.c *.cc) Do (Set t=%%i && Set Args=!Args!!t:%~dp0=!)
For %%i In (formats\zopflipng\zopflipng_bin.cc formats\zopfli\zopfli_bin.c) Do Set Args=!Args:%%i =!
g++ %Args%
Pause