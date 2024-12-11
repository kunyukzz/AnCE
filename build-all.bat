@ECHO OFF
REM build everything

ECHO "Build Everything..."

PUSHD engine
call build.bat
POPD
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && exit)

PUSHD testbed  
CALL build.bat
POPD
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && exit)
ECHO "All assemblies built successfully."