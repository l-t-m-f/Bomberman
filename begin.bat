@echo off
rem This script is to load a default set of submodules. It performs a full cleanup. To simply update, use the fetch.bat
rem This allows us to increment variables during loop
setlocal enabledelayedexpansion

set PLUTO=https://LetTheMiceFree@dev.azure.com/LetTheMiceFree/pluto/_git/pluto
rem Check if the repository directory already exists
    if exist deps\pluto (
        rem Directory exists, clear it
        echo Removing existing directory: deps/pluto
        rmdir /s /q deps\pluto || (
            echo Failed to remove existing directory: deps/pluto
            exit /b 1
        )
    )

rem Clone Pluto
    echo Cloning Pluto framework
    git clone !PLUTO! deps\pluto || (
        echo Failed to clone repository: deps/pluto
        exit /b 1
	)
		
set GAME_MODULES=https://LetTheMiceFree@dev.azure.com/LetTheMiceFree/game_modules/_git/game_modules
rem Check if the repository directory already exists
    if exist deps\game_modules (
        rem Directory exists, clear it
        echo Removing existing directory: deps/game_modules
        rmdir /s /q deps\game_modules || (
            echo Failed to remove existing directory: deps/game_modules
            exit /b 1
        )
    )
	
rem Clone Game modules
    echo Cloning additional game modules
    git clone !GAME_MODULES! deps\game_modules || (
        echo Failed to clone repository: deps/game_modules
        exit /b 1
	)

rem Define the list of repository URLs
set REPOS=l-t-m-f/SDL-ltmf libsdl-org/SDL_image libsdl-org/SDL_mixer libsdl-org/SDL_net libsdl-org/SDL_ttf dsprenkels/randombytes liteserver/binn Auburn/FastNoiseLite P-p-H-d/mlib SanderMertens/flecs

rem Define the list of repository names (this list must be in the same order as the URL list)
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

rem Loop through each repository and clear existing directories before adding them again
for /L %%i in (1,1,!i!) do (
    rem Get the corresponding URL and repository name
    set URL=https://github.com/!REPO[%%i]!
    set DEPS=!DEPS[%%i]!

    rem Check if the repository directory already exists
    if exist deps\!DEPS! (
        rem Directory exists, clear it
        echo Removing existing directory: deps/!DEPS!
        rmdir /s /q deps\!DEPS! || (
            echo Failed to remove existing directory: deps/!DEPS!
            exit /b 1
        )
    )

    rem Clone the repository
    echo Cloning repository: URL=!URL!, Path=deps/!DEPS!
    git clone !URL! deps\!DEPS! || (
        echo Failed to clone repository: deps/!DEPS!
        exit /b 1
    )
)

cd deps
cd SDL
git remote set-head origin custom
echo SDL set to custom branch on l-t-m-f/SDL-ltmf fork

endlocal
