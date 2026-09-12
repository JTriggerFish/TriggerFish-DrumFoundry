[CmdletBinding()]
param(
    [ValidateSet('setup','doctor','build','test','python-test','test-fitting-tools','check','dist')]
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
        'test' { Build-Native; Invoke-Checked ctest @('--test-dir', 'build/native', '--output-on-failure') }
        'python-test' { Invoke-PythonTests }
        'test-fitting-tools' { Invoke-PythonTests }
        'check' { Invoke-Checked (Resolve-DevelopmentPython) @('-m','pre_commit','run','--all-files') }
        'dist' {
            Build-Native
            Invoke-Checked cpack @('--config','build/native/CPackConfig.cmake','-B','dist')
        }
    }
} finally { Pop-Location }
