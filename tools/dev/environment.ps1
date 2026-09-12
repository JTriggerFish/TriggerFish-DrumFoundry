# Shared development environment; never discover or select Visual Studio.
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE) { throw "$Program failed with exit code $LASTEXITCODE" }
}

function Initialize-NativeEnvironment {
    if ([Environment]::OSVersion.Platform -eq 'Win32NT') {
        $msys = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { 'C:/msys64' }
        $bin = Join-Path $msys 'mingw64/bin'
        foreach ($tool in @('g++.exe','ninja.exe','cmake.exe')) {
            if (-not (Test-Path -LiteralPath (Join-Path $bin $tool))) {
                throw "Missing MinGW tool: $bin/$tool. See DEVELOPMENT.md."
            }
        }
        $env:PATH = "$bin;$env:PATH"
    }
}

function Resolve-DevelopmentPython {
    if ($Python) { return $Python }
    $relative = if ([Environment]::OSVersion.Platform -eq 'Win32NT') { '.venv/Scripts/python.exe' } else { '.venv/bin/python' }
    $path = Join-Path $repoRoot $relative
    if (-not (Test-Path -LiteralPath $path)) { throw 'Run ./dev.ps1 setup first' }
    return $path
}

function Show-DevelopmentEnvironment {
    Initialize-NativeEnvironment
    Invoke-Checked cmake @('--version')
    Invoke-Checked ninja @('--version')
    $compiler = if ([Environment]::OSVersion.Platform -eq 'Win32NT') { 'g++' } else { 'c++' }
    Invoke-Checked $compiler @('--version')
    Invoke-Checked git @('status','--short','--branch')
}
