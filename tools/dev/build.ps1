# Native builds have no Python, Node, Wasm, Rack or GUI dependency.
function Build-Native([bool]$WithClap = $false, [bool]$WithStandalone = $false, [bool]$WithUi = $false) {
    Initialize-NativeEnvironment
    $arguments = @('--preset','native')
    if ([Environment]::OSVersion.Platform -eq 'Win32NT') {
        $arguments = @('--preset','mingw')
    }
    $clapOption = if ($WithClap) { 'ON' } else { 'OFF' }
    $standaloneOption = if ($WithStandalone) { 'ON' } else { 'OFF' }
    $uiOption = if ($WithUi) { 'ON' } else { 'OFF' }
    Invoke-Checked cmake ($arguments + @("-DDRUMFOUNDRY_BUILD_CLAP=$clapOption", "-DDRUMFOUNDRY_BUILD_STANDALONE=$standaloneOption", "-DDRUMFOUNDRY_BUILD_UI=$uiOption"))
    Invoke-Checked cmake @('--build','build/native','--parallel',"$Jobs")
}

function Invoke-PythonTests {
    Build-Native
    $previousPath = $env:PYTHONPATH
    try {
        $env:PYTHONPATH = Join-Path $repoRoot 'python'
        Invoke-Checked (Resolve-DevelopmentPython) @('-m','pytest','tests/python')
    } finally { $env:PYTHONPATH = $previousPath }
}
