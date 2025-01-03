echo "Building EDIE 32-bit"
set EDIE_PLATFORM=win32
del novatel_edie\resources\*.dll
pause
python setup.py clean --all bdist_wheel -p win32
pause
echo "Building EDIE 64-bit"
set EDIE_PLATFORM=win64
del novatel_edie\resources\*.dll
pause
python setup.py clean --all bdist_wheel -p win_amd64
