REM
@ECHO OFF
SetLocal EnableDelayedExpansion

REM Get list of all the .c files
SET cFilenames=
FOR /R %%f in (*c) do (
    SET cFilenames=!cFilenames! %%f
)

REM echo "Files:" %cFilenames%
SET assembly=engine
SET compilerFlags=-g -shared -Wvarargs -Wall -Werror
REM -Wall -Werror
SET includeFlags=-Isrc
SET linkerFlags=-luser32
SET defines=-D_DEBUG -DACEXPORT -D_CRT_SECURE_NO_WARNING

ECHO "Building %assembly%%...."
clang %cFilenames% %compilerFlags% -o ../bin/%assembly%.dll %defines% %includeFlags% %linkerFlags%