@echo off
setlocal

rem Define an array of SDL package names
set DEPS=SDL SDL_image SDL_mixer SDL_ttf SDL_net binn fnl flecs mlib randombytes game_modules Pluto

rem Loop through each package and perform git pull
for %%P in (%DEPS%) do (
        rem Handle the other SDL packages
        cd deps\%%P
    echo Pulling latest changes from %%P...
    git fetch origin
    git merge origin/HEAD
    echo Finished pulling %%P

    rem Return to the original directory
    cd ..
    cd ..
)

endlocal