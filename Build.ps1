param([string]$JucePath = '')
$ErrorActionPreference = 'Stop'
function Invoke-CMake([string[]]$Arguments) {
    $start = [System.Diagnostics.ProcessStartInfo]::new('C:\Program Files\CMake\bin\cmake.exe')
    $start.UseShellExecute = $false
    $savedPath = $env:PATH
    [void]$start.Environment.Remove('Path')
    [void]$start.Environment.Remove('PATH')
    $start.Environment['PATH'] = $savedPath
    foreach ($arg in $Arguments) { $start.ArgumentList.Add($arg) }
    $process = [System.Diagnostics.Process]::Start($start)
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "CMake failed ($($process.ExitCode))" }
}
$configure = @('-S', $PSScriptRoot, '-B', "$PSScriptRoot/out", '-G', 'Visual Studio 18 2026', '-A', 'x64')
$configure += "-DJUCE_PATH=$JucePath"
Invoke-CMake $configure
Invoke-CMake @('--build', "$PSScriptRoot/out", '--config', 'Release', '--parallel', '4')
