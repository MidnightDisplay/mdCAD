#------------------------------------------------------------------------------
# build-all.ps1 - Complete Vulkan shader build pipeline
#
# Usage: .\build-all.ps1
#
# This script runs the complete shader compilation pipeline:
#   1. Compile GLSL 450 sources to SPIR-V bytecode
#   2. Generate C header with embedded bytecode arrays
#
# Prerequisites:
#   - Vulkan SDK installed with VULKAN_SDK environment variable set
#   - GLSL source files in src/shaders/spirv/*.vert and *.frag
#------------------------------------------------------------------------------

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Vulkan Shader Build Pipeline" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Compile GLSL to SPIR-V
Write-Host "Step 1: Compiling GLSL to SPIR-V" -ForegroundColor Yellow
Write-Host "----------------------------------------"
& "$scriptDir\compile-spirv.ps1"

if ($LASTEXITCODE -ne 0) {
    Write-Error "SPIR-V compilation failed"
    exit 1
}

Write-Host ""

# Step 2: Generate C header
Write-Host "Step 2: Generating C bytecode header" -ForegroundColor Yellow
Write-Host "----------------------------------------"
& "$scriptDir\generate-bytecode-header.ps1"

if ($LASTEXITCODE -ne 0) {
    Write-Error "Header generation failed"
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Shader build complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Rebuild the project: cmake --build build-mingw"
Write-Host "  2. Test the application: .\build-mingw\bin\mdCAD.exe"
