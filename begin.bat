@echo off
rem This script is to load a default set of submodules. It performs a full cleanup. To simply update, use the fetch.bat
rem This allows us to increment variables during loop
setlocal enabledelayedexpansion

rem Define the list of repository URLs
set REPOS=l-t-m-f/SDL-ltmf libsdl-org/SDL_image libsdl-org/SDL_mixer libsdl-org/SDL_net libsdl-org/SDL_ttf dsprenkels/randombytes liteserver/binn Auburn/FastNoiseLite P-p-H-d/mlib SanderMertens/flecs

rem Define the list of submodule names (this list must be in the same order as the URL list)
set DEPS_LIST=SDL SDL_image SDL_mixer SDL_net SDL_ttf randombytes binn fnl mlib flecs

rem Convert the REPOS list into an array
set i=0
for %%A in (%REPOS%) do (
    set /a i+=1
    set REPO[!i!]=%%A
)

rem Convert the DEPS_LIST list into an array
set j=0
for %%B in (%DEPS_LIST%) do (
    set /a j+=1
    set DEPS[!j!]=%%B
)

rem Loop through each submodule and clear existing directories before adding them again
for /L %%i in (1,1,!i!) do (
    rem Get the corresponding URL and submodule name
    set URL=https://github.com/!REPO[%%i]!
    set DEPS=!DEPS[%%i]!

    rem Check if the submodule directory already exists
    if exist deps\!DEPS! (
        rem Directory exists, clear it
        echo Removing existing directory: deps/!DEPS!
        rmdir /s /q deps\!DEPS! || (
            echo Failed to remove existing directory: deps/!DEPS!
            exit /b 1
        )
    )

    rem Clone the submodule
    echo Cloning submodule: URL=!URL!, Path=deps/!DEPS!
    git clone !URL! deps\!DEPS! || (
        echo Failed to clone submodule: deps/!DEPS!
        exit /b 1
    )
)

endlocal
