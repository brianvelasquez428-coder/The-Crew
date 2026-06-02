@echo off
echo [SYSTEM] Compiling The Crew with Raylib Graphics...

:: Jump straight to the compile step
goto CompileStep

-------------------------------------------------------------------
HOW THIS SCRIPT WORKS:
-I tells the compiler where to find our headers AND Raylib's headers.
-L tells the compiler where Raylib's pre-built engine lives.
-l (lowercase L) tells the compiler to attach Windows graphics drivers.
-static packs the C++ background files into the exe so friends can play it!
-------------------------------------------------------------------

:CompileStep
g++ -I Core -I Combat -I Data -I Systems -I C:\raylib\include Core\*.cpp Combat\*.cpp Data\*.cpp Systems\*.cpp -o TheCrew.exe -L C:\raylib\lib -lraylib -lopengl32 -lgdi32 -lwinmm -static

if %errorlevel% neq 0 (
    echo [SYSTEM] Build Failed! Check your code.
    exit /b %errorlevel%
)

cls
TheCrew.exe


:: cd "The Crew" ; .\build.bat


