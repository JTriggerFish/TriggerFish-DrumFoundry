[CmdletBinding()]
param(
    [ValidateSet('setup','doctor','build','test','clap','clap-test','clap-dist','standalone','standalone-test','standalone-dist','ui','ui-test','ui-dist','python-test','test-fitting-tools','perceptual-test','benchmark-percussion','check','dist')]
    [string]$Command = 'build',
    [ValidateRange(1,64)][int]$Jobs = 4,
    [string]$Python = ''
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = $PSScriptRoot
. "$repoRoot/tools/dev/environment.ps1"
. "$repoRoot/tools/dev/build.ps1"
. "$repoRoot/tools/dev/setup.ps1"
Push-Location $repoRoot
try {
    switch ($Command) {
        'setup' { Initialize-Development }
        'doctor' { Show-DevelopmentEnvironment }
        'build' { Build-Native }
        'clap' { Build-Native $true }
        'ui' { Build-Native $true $true $true }
        'ui-test' { Build-Native $true $true $true; Invoke-Checked ctest @('--test-dir','build/native','--output-on-failure') }
        'ui-dist' {
            Build-Native $true $true $true
            Invoke-Checked cpack @('--config','build/native/CPackConfig.cmake','-B','dist/ui')
        }
        'standalone' { Build-Native $true $true }
        'standalone-test' { Build-Native $true $true; Invoke-Checked ctest @('--test-dir','build/native','--output-on-failure') }
        'standalone-dist' {
            Build-Native $true $true
            Invoke-Checked cpack @('--config','build/native/CPackConfig.cmake','-B','dist/standalone')
        }
        'clap-test' { Build-Native $true; Invoke-Checked ctest @('--test-dir','build/native','--output-on-failure') }
        'clap-dist' {
            Build-Native $true
            Invoke-Checked cpack @('--config','build/native/CPackConfig.cmake','-B','dist/clap')
        }
        'test' { Build-Native; Invoke-Checked ctest @('--test-dir', 'build/native', '--output-on-failure') }
        'python-test' { Invoke-PythonTests }
        'test-fitting-tools' { Invoke-PythonTests }
        'perceptual-test' {
            Invoke-Checked (Resolve-DevelopmentPython) @('-m','pytest','tests/perceptual','tests/python/test_reference_floor_mel.py','tests/python/test_metal_refinement.py')
        }
        'benchmark-percussion' {
            Build-Native
            Invoke-Checked cmake @('--build','build/native','--target','percussion_benchmark','--parallel',"$Jobs")
            Invoke-Checked (Join-Path $repoRoot 'build/native/tests/percussion_benchmark') @()
        }
        'check' { Invoke-Checked (Resolve-DevelopmentPython) @('-m','pre_commit','run','--all-files') }
        'dist' {
            Build-Native
            Invoke-Checked cpack @('--config','build/native/CPackConfig.cmake','-B','dist')
        }
    }
} finally { Pop-Location }
