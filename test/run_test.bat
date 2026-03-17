@echo off
setlocal

set "ROOT=C:\Users\jairs\Claude\MP3Gain"
set "TMP=%TEMP%\mp3gain-run-test"

if exist "%TMP%" rmdir /s /q "%TMP%"
mkdir "%TMP%"
copy /y "%ROOT%\test\fixtures\test1.mp3" "%TMP%\test1.mp3" >nul

cd /d "%ROOT%\build\Release"
mp3gain.exe "%TMP%\test1.mp3" > "%ROOT%\test\out.txt" 2>&1
echo EXIT=%ERRORLEVEL% >> "%ROOT%\test\out.txt"
