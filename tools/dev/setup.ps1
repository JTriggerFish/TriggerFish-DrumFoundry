# Repository-local Python environment and hooks; never alter global Git identity.
function Initialize-Development {
    Invoke-Checked uv @('sync','--locked','--group','dev','--python','3.13')
    Invoke-Checked (Resolve-DevelopmentPython) @('-m','pre_commit','install')
    Invoke-Checked git @('config','--local','core.autocrlf','false')
    Invoke-Checked git @('config','--local','core.eol','lf')
    Write-Host 'Development environment ready. Run ./dev.ps1 test and ./dev.ps1 python-test.'
}
