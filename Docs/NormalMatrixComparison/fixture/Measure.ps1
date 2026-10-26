$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$images = Join-Path $repo 'Docs/NormalMatrixComparison'
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
public static class CaptureDifference {
    public static double[] Measure(byte[] a, byte[] b) {
        long changed=0, total=0; int max=0;
        for(int i=0;i<a.Length;i+=4) {
            bool different=false;
            for(int c=0;c<3;c++) {
                int d=Math.Abs(a[i+c]-b[i+c]);
                total+=d; max=Math.Max(max,d); different|=d!=0;
            }
            if(different) changed++;
        }
        return new double[]{changed, total/(a.Length/4.0*3), max};
    }
}
'@
function Read-Pixels([string]$path) {
    $bitmap = [System.Drawing.Bitmap]::new($path)
    $rect = [System.Drawing.Rectangle]::new(0,0,$bitmap.Width,$bitmap.Height)
    $locked = $bitmap.LockBits($rect,[System.Drawing.Imaging.ImageLockMode]::ReadOnly,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bytes = [byte[]]::new([Math]::Abs($locked.Stride)*$bitmap.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($locked.Scan0,$bytes,0,$bytes.Length)
    $bitmap.UnlockBits($locked)
    $bitmap.Dispose()
    return ,$bytes
}
$results = foreach ($condition in @('uniform','nonuniform')) {
    $a = Read-Pixels "$images\legacy-$condition.png"
    $b = Read-Pixels "$images\current-$condition.png"
    $m = [CaptureDifference]::Measure($a,$b)
    [pscustomobject]@{condition=$condition; changedPixels=$m[0]; totalPixels=$a.Length/4; meanAbsoluteRGB8Difference=$m[1]; maxRGB8Difference=$m[2]}
}
$results | ConvertTo-Json | Set-Content -LiteralPath "$images\pixel-comparison.json" -Encoding utf8
$results | Format-Table -AutoSize
$files = @(
    'Engine/Math/Matrix4.cpp', 'Engine/Renderer/RenderLayer/OpaqueLayer.cpp',
    'Engine/Shader/MeshVertexShader.hlsl', 'Game/Actor/TestMeshActor.cpp',
    'Bin/NormalMatrixComparison/ComparisonMain.cpp', 'Bin/NormalMatrixComparison/LegacyOpaqueLayer.cpp',
    'Bin/NormalMatrixComparison/Current/Game.exe', 'Bin/NormalMatrixComparison/Legacy/Game.exe',
    'Bin/NormalMatrixComparison/Current/Engine.dll', 'Bin/NormalMatrixComparison/Legacy/Engine.dll',
    'Bin/NormalMatrixComparison/Current/Shader/MeshVertexShader.cso',
    'Bin/NormalMatrixComparison/Legacy/Shader/MeshVertexShader.cso',
    'Docs/NormalMatrixComparison/current-uniform.png', 'Docs/NormalMatrixComparison/legacy-uniform.png',
    'Docs/NormalMatrixComparison/current-nonuniform.png', 'Docs/NormalMatrixComparison/legacy-nonuniform.png'
)
$hashes = foreach ($relative in $files) {
    [pscustomobject]@{path=$relative; sha256=(Get-FileHash -LiteralPath (Join-Path $repo $relative) -Algorithm SHA256).Hash}
}
[pscustomobject]@{capturedAt=(Get-Date -Format o); baseCommit=(git rev-parse HEAD); configuration='Release x64'; resolution='960x640'; files=$hashes} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$images\manifest.json" -Encoding utf8
