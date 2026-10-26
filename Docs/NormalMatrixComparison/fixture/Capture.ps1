param([ValidateSet('Current','Legacy')][string]$Mode, [switch]$Uniform)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$runtime = Join-Path $PSScriptRoot $Mode
$output = Join-Path $repo 'Docs/NormalMatrixComparison'
New-Item -ItemType Directory -Path $output -Force | Out-Null
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class GameCapture {
    public delegate bool EnumProc(IntPtr hwnd, IntPtr param);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc callback, IntPtr param);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd, out Rect rect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr dc, uint flags);
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int left,top,right,bottom; }
    public static IntPtr Find(uint pid) {
        IntPtr result = IntPtr.Zero;
        EnumWindows((hwnd,p) => {
            uint owner; GetWindowThreadProcessId(hwnd,out owner);
            Rect r;
            if(owner==pid && IsWindowVisible(hwnd) && GetClientRect(hwnd,out r) && r.right>500 && r.bottom>300)
                result = hwnd;
            return true;
        },IntPtr.Zero);
        return result;
    }
}
'@
$launch = @{FilePath=(Join-Path $runtime 'Game.exe'); WorkingDirectory=$runtime; WindowStyle='Normal'; PassThru=$true}
if ($Uniform) { $launch.ArgumentList='--uniform' }
$game = Start-Process @launch
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    $window = [IntPtr]::Zero
    do {
        Start-Sleep -Milliseconds 200
        $game.Refresh()
        if ($game.HasExited) { throw "Game exited early: $($game.ExitCode)" }
        $window = [GameCapture]::Find($game.Id)
    } while ($window -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
    if ($window -eq [IntPtr]::Zero) { throw 'Game window not found' }
    Start-Sleep -Seconds 2
    $rect = New-Object GameCapture+Rect
    if (-not [GameCapture]::GetClientRect($window,[ref]$rect)) { throw 'Client bounds unavailable' }
    $bitmap = New-Object System.Drawing.Bitmap($rect.right,$rect.bottom)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $dc = $graphics.GetHdc()
    try { $captured = [GameCapture]::PrintWindow($window,$dc,3) }
    finally { $graphics.ReleaseHdc($dc); $graphics.Dispose() }
    if (-not $captured) { $bitmap.Dispose(); throw 'PrintWindow failed' }
    $condition = if ($Uniform) { 'uniform' } else { 'nonuniform' }
    $path = Join-Path $output "$($Mode.ToLower())-$condition.png"
    $bitmap.Save($path,[System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
    Write-Output "Saved $path ($($rect.right)x$($rect.bottom)); PID=$($game.Id)"
} finally {
    $game.Refresh()
    if (-not $game.HasExited) {
        $null=$game.CloseMainWindow()
        if (-not $game.WaitForExit(3000)) { Stop-Process -Id $game.Id }
    }
}
