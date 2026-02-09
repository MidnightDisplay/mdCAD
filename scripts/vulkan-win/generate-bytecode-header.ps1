#------------------------------------------------------------------------------
# generate-bytecode-header.ps1 - Convert SPIR-V files to C header with byte arrays
#
# Usage: .\generate-bytecode-header.ps1
#
# Prerequisites:
#   - SPIR-V bytecode files in src/shaders/spirv/*.spv (run compile-spirv.ps1 first)
#
# Output:
#   - C header file: src/shaders/spirv/spirv_bytecode.h
#------------------------------------------------------------------------------

$ErrorActionPreference = "Stop"

# Paths
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent (Split-Path -Parent $scriptDir)
$shaderDir = Join-Path $projectRoot "src\shaders\spirv"
$outputFile = Join-Path $shaderDir "spirv_bytecode.h"

Write-Host "Generating SPIR-V bytecode header..."
Write-Host "Shader directory: $shaderDir"
Write-Host "Output file: $outputFile"
Write-Host ""

# Function to convert binary file to C array
function ConvertTo-CArray {
    param (
        [string]$FilePath,
        [string]$ArrayName
    )
    
    $bytes = [System.IO.File]::ReadAllBytes($FilePath)
    $size = $bytes.Length
    
    $sb = [System.Text.StringBuilder]::new()
    [void]$sb.AppendLine("static const uint8_t ${ArrayName}[$size] = {")
    
    for ($i = 0; $i -lt $bytes.Length; $i += 16) {
        [void]$sb.Append("    ")
        $end = [Math]::Min($i + 16, $bytes.Length)
        for ($j = $i; $j -lt $end; $j++) {
            [void]$sb.AppendFormat("0x{0:x2}", $bytes[$j])
            if ($j -lt $bytes.Length - 1) {
                [void]$sb.Append(", ")
            }
        }
        [void]$sb.AppendLine()
    }
    
    [void]$sb.AppendLine("};")
    return $sb.ToString()
}

# Start building the header
$header = [System.Text.StringBuilder]::new()

[void]$header.AppendLine("#ifndef SPIRV_BYTECODE_H")
[void]$header.AppendLine("#define SPIRV_BYTECODE_H")
[void]$header.AppendLine()
[void]$header.AppendLine("#include <stdint.h>")
[void]$header.AppendLine()

# Process all .spv files
$spvFiles = Get-ChildItem -Path $shaderDir -Filter "*.spv" | Sort-Object Name

if ($spvFiles.Count -eq 0) {
    Write-Error "No .spv files found in $shaderDir. Run compile-spirv.ps1 first."
    exit 1
}

foreach ($spv in $spvFiles) {
    # Convert filename to array name: instanced_line_vs.spv -> instanced_line_vs_spirv
    $arrayName = $spv.BaseName + "_spirv"
    
    Write-Host "  Processing: $($spv.Name) -> $arrayName"
    
    $arrayCode = ConvertTo-CArray -FilePath $spv.FullName -ArrayName $arrayName
    [void]$header.AppendLine($arrayCode)
}

[void]$header.AppendLine("#endif /* SPIRV_BYTECODE_H */")

# Write the header file
$header.ToString() | Out-File -FilePath $outputFile -Encoding utf8 -NoNewline

$fileSize = (Get-Item $outputFile).Length
Write-Host ""
Write-Host "Generated $outputFile ($fileSize bytes)" -ForegroundColor Green
Write-Host "Processed $($spvFiles.Count) SPIR-V files"
