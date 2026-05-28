@echo off
setlocal

REM Comprobar si se ha pasado al menos un parámetro
if "%~1"=="" (
    echo Error: Faltan parametros.
    echo Uso: %~nx0 ^<archivo_fuente.c^> [nombre_ejecutable]
    exit /b 1
)

set "ARCHIVO_FUENTE=%~1"

REM Deducir el nombre del .exe
if "%~2"=="" (
    set "NOMBRE_EJECUTABLE=%~n1.exe"
) else (
    set "NOMBRE_EJECUTABLE=%~2"
)

echo ========================================
echo Compilando: %ARCHIVO_FUENTE%
echo Salida esperada: %NOMBRE_EJECUTABLE%
echo ========================================

REM Crear carpeta bin si no existe
if not exist "bin" (
    mkdir bin
)

REM Compilar
gcc "src\%ARCHIVO_FUENTE%" -o "bin\%NOMBRE_EJECUTABLE%"

REM Validar error de compilación (neq significa "no es igual a")
if %errorlevel% neq 0 (
    echo [X] Error en la compilacion. Revisa tu codigo en "src\%ARCHIVO_FUENTE%.
    exit /b 1
)

REM Si llega hasta aquí, la compilación fue un éxito (¡sin paréntesis problemáticos!)
echo [OK] Compilacion exitosa.

set /p "respuesta=Deseas ejecutarlo ahora? (s/n): "

if /i "%respuesta%"=="s" goto ejecutar

goto fin

:ejecutar
echo ========================================
echo Ejecutando %NOMBRE_EJECUTABLE%...
echo ========================================
"bin\%NOMBRE_EJECUTABLE%"

:fin
endlocal