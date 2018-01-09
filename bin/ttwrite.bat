set arduino_stm32="E:\Arduino\IDE\arduino-1.8.3"
set dev=COM4

@echo off
PATH=%PATH%;%arduino_stm32%\java\bin;
set driverLetter=%~dp0
set driverLetter=%driverLetter:~0,2%
%driverLetter%
cd %~dp0
java -jar maple_loader.jar %dev% 2 1EAF:0003 %1

REM for /l %%x in (1, 1, 40) do (
REM   ping -w 50 -n 1 192.0.2.1 > nul
REM mode %1 > nul
REM   if ERRORLEVEL 0 goto comPortFound
REM )

echo timeout waiting for %dev% serial

:comPortFound
