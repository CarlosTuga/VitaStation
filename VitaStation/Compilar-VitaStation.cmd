@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-vita.ps1" %*
set "BUILD_RESULT=%ERRORLEVEL%"
if not "%BUILD_RESULT%"=="0" echo Falha na compilacao. Consulta as mensagens acima.
pause
exit /b %BUILD_RESULT%
