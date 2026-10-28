param(
    [string]$SdkPath = 'D:\Vita sdk',
    [string]$BuildDirectory = (Join-Path $PSScriptRoot 'build-vita')
)
$ErrorActionPreference = 'Stop'
$oldSdk = $env:VITASDK
$oldPath = $env:PATH
try {
    $sdk = (Resolve-Path -LiteralPath $SdkPath).Path
    foreach ($relative in @('bin\arm-vita-eabi-gcc.exe', 'share\vita.toolchain.cmake', 'share\vita.cmake', 'arm-vita-eabi\lib\libvita2d.a', 'arm-vita-eabi\lib\libfreetype.a', 'arm-vita-eabi\lib\libbz2.a')) {
        if (!(Test-Path -LiteralPath (Join-Path $sdk $relative))) { throw "Falta no SDK: $relative" }
    }
    $cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($cmakeCommand) { $cmakeExe = $cmakeCommand.Source }
    else {
        $cmakeBin = & python -c 'import cmake; print(cmake.CMAKE_BIN_DIR)'
        if ($LASTEXITCODE -ne 0) { throw 'CMake indisponivel. Instala CMake ou o pacote Python cmake.' }
        $cmakeExe = Join-Path $cmakeBin 'cmake.exe'
    }
    $ninjaCommand = Get-Command ninja.exe -ErrorAction SilentlyContinue
    if ($ninjaCommand) { $ninjaExe = $ninjaCommand.Source }
    else {
        $ninjaBin = & python -c 'import ninja; print(ninja.BIN_DIR)'
        if ($LASTEXITCODE -ne 0) { throw 'Ninja indisponivel. Instala Ninja ou o pacote Python ninja.' }
        $ninjaExe = Join-Path $ninjaBin 'ninja.exe'
    }
    $env:VITASDK = $sdk
    $env:PATH = (Join-Path $sdk 'bin') + ';' + (Split-Path $cmakeExe) + ';' + (Split-Path $ninjaExe) + ';' + $oldPath
    & $cmakeExe -S $PSScriptRoot -B $BuildDirectory -G Ninja -DVITA=ON "-DCMAKE_MAKE_PROGRAM=$ninjaExe"
    if ($LASTEXITCODE -ne 0) { throw 'Falhou a configuracao CMake.' }
    & $cmakeExe --build $BuildDirectory --parallel 2
    if ($LASTEXITCODE -ne 0) { throw 'Falhou a compilacao Vita.' }
    $vpk = Join-Path $BuildDirectory 'VitaStation.vpk'
    if (!(Test-Path -LiteralPath $vpk)) { throw 'O build terminou sem o VPK esperado.' }
    & python (Join-Path $PSScriptRoot 'tests\validate_vpk.py') $vpk
    if ($LASTEXITCODE -ne 0) { throw 'O VPK falhou a validacao dos recursos de instalacao.' }
    Write-Host "VPK criado: $vpk" -ForegroundColor Green
}
finally {
    $env:VITASDK = $oldSdk
    $env:PATH = $oldPath
}
