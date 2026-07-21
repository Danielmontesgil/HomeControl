@echo off
set "PATH=C:\Qt\6.11.1\msvc2022_64\bin;%PATH%"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 (
    echo Error al inicializar vcvarsall.bat x64.
    exit /b %errorlevel%
)
echo [TestGuard] Configurando CMake...
cmake -B cmake-build-debug -G Ninja
if %errorlevel% neq 0 (
    echo Error en la configuracion de CMake.
    exit /b %errorlevel%
)
echo [TestGuard] Compilando HomeControl (Produccion)...
cmake --build cmake-build-debug --target HomeControl
if %errorlevel% neq 0 (
    echo Error en la compilacion de HomeControl.
    exit /b %errorlevel%
)
echo [TestGuard] Compilando HomeControlTests (Pruebas)...
cmake --build cmake-build-debug --target HomeControlTests
if %errorlevel% neq 0 (
    echo Error en la compilacion de HomeControlTests.
    exit /b %errorlevel%
)
echo [TestGuard] Ejecutando pruebas unitarias...
.\cmake-build-debug\HomeControlTests.exe --gtest_output=json:test_results.json
if %errorlevel% neq 0 (
    echo Alguna prueba fallo o error de ejecucion (Codigo de salida: %errorlevel%).
    exit /b %errorlevel%
)
echo [TestGuard] Proceso completado con exito.
