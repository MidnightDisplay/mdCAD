param(
    [string]$SourceDir = "build-vulkan\bin\Release",
    [string]$TargetDir = "samples\avalonia-mdcad-control\runtime\win-x64"
)

$sourcePath = Join-Path (Get-Location) $SourceDir
$targetPath = Join-Path (Get-Location) $TargetDir

if (!(Test-Path $sourcePath -PathType Container)) {
    throw "Source runtime directory not found: $sourcePath"
}

$requiredFiles = @(
    "mdCAD.exe",
    "imgui.embedded.ini"
)

$optionalPatterns = @(
    "*.dll",
    "*.json",
    "*.pak",
    "*.dat",
    "*.bin",
    "*.spv"
)

New-Item -ItemType Directory -Force -Path $targetPath | Out-Null
Get-ChildItem -Path $targetPath -File -ErrorAction SilentlyContinue | Remove-Item -Force

foreach ($fileName in $requiredFiles) {
    $sourceFile = Join-Path $sourcePath $fileName
    if (!(Test-Path $sourceFile -PathType Leaf)) {
        throw "Required runtime file not found: $sourceFile"
    }

    Copy-Item -Path $sourceFile -Destination (Join-Path $targetPath $fileName) -Force
}

foreach ($pattern in $optionalPatterns) {
    Get-ChildItem -Path $sourcePath -Filter $pattern -File -ErrorAction SilentlyContinue |
        ForEach-Object {
            Copy-Item -Path $_.FullName -Destination (Join-Path $targetPath $_.Name) -Force
        }
}

Write-Host "Pinned mdCAD runtime refreshed into $targetPath"
