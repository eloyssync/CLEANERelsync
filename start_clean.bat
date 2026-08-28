@echo off
chcp 65001 > nul
cd /d "%~dp0"

echo Поиск окружения Visual Studio...
set "VS_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%VS_PATH%" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

if not exist "%VS_PATH%" (
    echo [ОШИБКА] Не найден vcvars64.bat! Проверь путь к Visual Studio.
    pause
    exit /b 1
)

call "%VS_PATH%"

echo Генерация файла ресурсов (иконка + метаданные)...
(
echo 1 ICON "logo.ico"
echo 1 VERSIONINFO
echo FILEVERSION 2,0,0,0
echo PRODUCTVERSION 2,0,0,0
echo FILEFLAGSMASK 0x3fL
echo FILEFLAGS 0x0L
echo FILEOS 0x40004L
echo FILETYPE 0x1L
echo FILESUBTYPE 0x0L
echo BEGIN
echo     BLOCK "StringFileInfo"
echo     BEGIN
echo         BLOCK "040904b0"
echo         BEGIN
echo             VALUE "CompanyName", "eloysync"
echo             VALUE "FileDescription", "CLEANERelsync - Python Source Code Formatter"
echo             VALUE "FileVersion", "2.0.0.0"
echo             VALUE "InternalName", "CLEANERelsync"
echo             VALUE "OriginalFilename", "CLEANERelsync.exe"
echo             VALUE "ProductName", "CLEANERelsync"
echo             VALUE "ProductVersion", "2.0.0.0"
echo             VALUE "LegalCopyright", "Copyright 2026 eloysync"
echo         END
echo     END
echo     BLOCK "VarFileInfo"
echo     BEGIN
echo         VALUE "Translation", 0x409, 1200
echo     END
echo END
) > resource.rc

echo Компиляция ресурсов...
rc /r resource.rc

echo Сборка безопасного бинарника C++...
cl /utf-8 /EHsc /std:c++17 /O2 /MT /GS /guard:cf /Fe:CLEANERelsync.exe main.cpp resource.res /link /SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT /OPT:REF /OPT:ICF

if %errorlevel% equ 0 (
    echo.
    echo ========================================================
    echo УСПЕШНО! Файл CLEANERelsync.exe собран с защитой и инфой.
    echo ========================================================
) else (
    echo.
    echo [ОШИБКА] Сборка провалилась!
)

pause