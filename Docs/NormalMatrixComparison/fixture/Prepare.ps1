$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$work = $PSScriptRoot
foreach ($name in @('Current','Legacy','LegacyEngine','obj-game','obj-engine')) {
    New-Item -ItemType Directory -Path (Join-Path $work $name) -Force | Out-Null
}

function Copy-Project([string]$source, [string]$destination, [bool]$engine) {
    [xml]$project = Get-Content -LiteralPath $source -Raw
    $sourceDir = Split-Path $source
    foreach ($node in $project.SelectNodes('//*[@Include]')) {
        if ($node.LocalName -in @('ClCompile','ClInclude','FxCompile')) {
            $node.SetAttribute('Include', (Join-Path $sourceDir $node.GetAttribute('Include')))
        }
    }
    foreach ($node in $project.SelectNodes('//*[local-name()="AdditionalIncludeDirectories"]')) {
        $node.InnerText = "$repo\Engine;$repo\Game;" + $node.InnerText
    }
    if ($engine) {
        foreach ($node in $project.SelectNodes('//*[local-name()="FxCompile"]')) {
            $null = $node.ParentNode.RemoveChild($node)
        }
        foreach ($node in $project.SelectNodes('//*[local-name()="ClCompile"][@Include]')) {
            if ($node.GetAttribute('Include').EndsWith('RenderLayer\OpaqueLayer.cpp')) {
                $node.SetAttribute('Include', (Join-Path $work 'LegacyOpaqueLayer.cpp'))
            }
        }
    } else {
        foreach ($node in $project.SelectNodes('//*[local-name()="ClCompile"][@Include]')) {
            if ($node.GetAttribute('Include').EndsWith('\Main.cpp')) {
                $node.SetAttribute('Include', (Join-Path $work 'ComparisonMain.cpp'))
            }
        }
    }
    $project.Save($destination)
}

$legacy = Get-Content -LiteralPath "$repo\Engine\Renderer\RenderLayer\OpaqueLayer.cpp" -Raw
$legacy = $legacy.Replace('#include "OpaqueLayer.h"', '#include "Renderer/RenderLayer/OpaqueLayer.h"')
if (-not $legacy.Contains('&normalMatrix, sizeof(Matrix4)')) {
    throw 'Historical fixture requires the pre-ObjectConstantBuffer source. See README.md; refusing to build two identical variants.'
}
$legacy = $legacy.Replace('&normalMatrix, sizeof(Matrix4)', '&command.worldMatrix, sizeof(Matrix4)')
Set-Content -LiteralPath "$work\LegacyOpaqueLayer.cpp" -Value $legacy -Encoding utf8
Copy-Project "$repo\Engine\Engine.vcxproj" "$work\LegacyEngine.vcxproj" $true
Copy-Project "$repo\Game\Game.vcxproj" "$work\ComparisonGame.vcxproj" $false

$msbuild = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
$timer = [System.Diagnostics.Stopwatch]::StartNew()
& $msbuild "$work\ComparisonGame.vcxproj" /p:Configuration=Release /p:Platform=x64 "/p:SolutionDir=$repo\" "/p:OutDir=$work\Current\" "/p:IntDir=$work\obj-game\" /p:TargetName=Game /nologo /v:quiet *> "$work\game-build.log"
if ($LASTEXITCODE -ne 0) { Get-Content "$work\game-build.log" -Tail 30; throw 'Comparison Game build failed' }
Write-Output "Comparison Game built in $($timer.Elapsed.TotalSeconds) seconds"
& $msbuild "$work\LegacyEngine.vcxproj" /p:Configuration=Release /p:Platform=x64 "/p:SolutionDir=$repo\" "/p:OutDir=$work\LegacyEngine\" "/p:IntDir=$work\obj-engine\" /p:TargetName=Engine /nologo /v:quiet *> "$work\legacy-build.log"
if ($LASTEXITCODE -ne 0) { Get-Content "$work\legacy-build.log" -Tail 30; throw 'Legacy comparison Engine build failed' }
foreach ($mode in @('Current','Legacy')) {
    $runtime = Join-Path $work $mode
    Copy-Item -LiteralPath "$repo\Bin\Release\Game\Shader" -Destination $runtime -Recurse -Force
    Copy-Item -LiteralPath "$repo\Bin\Release\Game\Setting" -Destination $runtime -Recurse -Force
    Set-Content -LiteralPath "$runtime\Setting\Settings.txt" -Value "FrameRate: 120`r`nWidth: 960`r`nHeight: 640" -Encoding utf8
}
Copy-Item -LiteralPath "$work\Current\Game.exe" -Destination "$work\Legacy\Game.exe" -Force
Copy-Item -LiteralPath "$work\Current\Asset" -Destination "$work\Legacy" -Recurse -Force
Copy-Item -LiteralPath "$work\LegacyEngine\Engine.dll" -Destination "$work\Legacy\Engine.dll" -Force
Write-Output "Comparison builds ready in $($timer.Elapsed.TotalSeconds) seconds"
