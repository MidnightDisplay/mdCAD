#------------------------------------------------------------------------------
# compile-spirv.ps1 - Compile GLSL 450 shaders to SPIR-V bytecode
#
# Usage: .\compile-spirv.ps1
#
# Prerequisites:
#   - Vulkan SDK installed with VULKAN_SDK environment variable set
#   - GLSL source files in src/shaders/spirv/*.vert and *.frag
#
# Output:
#   - SPIR-V bytecode files: src/shaders/spirv/*.spv
#------------------------------------------------------------------------------

$ErrorActionPreference = "Stop"

# Verify Vulkan SDK
if (-not $env:VULKAN_SDK) {
    Write-Error "VULKAN_SDK environment variable not set. Install Vulkan SDK from https://vulkan.lunarg.com/"
    exit 1
}

$glslc = "$env:VULKAN_SDK\Bin\glslc.exe"
if (-not (Test-Path $glslc)) {
    Write-Error "glslc not found at $glslc"
    exit 1
}

# Paths
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent (Split-Path -Parent $scriptDir)
$shaderDir = Join-Path $projectRoot "src\shaders\spirv"

Write-Host "Compiling GLSL 450 shaders to SPIR-V..."
Write-Host "Shader directory: $shaderDir"
Write-Host ""

# Track success/failure
$compiled = 0
$failed = 0

# Compile vertex shaders
Get-ChildItem -Path $shaderDir -Filter "*.vert" | ForEach-Object {
    $input = $_.FullName
    $baseName = $_.BaseName
    $output = Join-Path $shaderDir "${baseName}_vs.spv"
    
    Write-Host "  Compiling: $($_.Name) -> ${baseName}_vs.spv"
    
    try {
        & $glslc -fshader-stage=vert $input -o $output
        if ($LASTEXITCODE -eq 0) {
            $size = (Get-Item $output).Length
            Write-Host "    OK ($size bytes)" -ForegroundColor Green
            $script:compiled++
        } else {
            Write-Host "    FAILED (exit code $LASTEXITCODE)" -ForegroundColor Red
            $script:failed++
        }
    } catch {
        Write-Host "    ERROR: $_" -ForegroundColor Red
        $script:failed++
    }
}

# Compile fragment shaders
Get-ChildItem -Path $shaderDir -Filter "*.frag" | ForEach-Object {
    $input = $_.FullName
    $baseName = $_.BaseName
    $output = Join-Path $shaderDir "${baseName}_fs.spv"
    
    Write-Host "  Compiling: $($_.Name) -> ${baseName}_fs.spv"
    
    try {
        & $glslc -fshader-stage=frag $input -o $output
        if ($LASTEXITCODE -eq 0) {
            $size = (Get-Item $output).Length
            Write-Host "    OK ($size bytes)" -ForegroundColor Green
            $script:compiled++
        } else {
            Write-Host "    FAILED (exit code $LASTEXITCODE)" -ForegroundColor Red
            $script:failed++
        }
    } catch {
        Write-Host "    ERROR: $_" -ForegroundColor Red
        $script:failed++
    }
}

Write-Host ""
Write-Host "Compilation complete: $compiled succeeded, $failed failed"

if ($failed -gt 0) {
    exit 1
}
