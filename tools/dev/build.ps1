# Native builds have no Python, Node, Wasm, Rack or GUI dependency.
function Build-Native {
    Initialize-NativeEnvironment
    $arguments = @('--preset','native')
    if ([Environment]::OSVersion.Platform -eq 'Win32NT') {
        $arguments = @('--preset','mingw')
    }
    Invoke-Checked cmake $arguments
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
