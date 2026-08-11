param(
    [Parameter(Mandatory=$true)]
    [string]$InputPath,

    [Parameter(Mandatory=$true)]
    [string]$OutputPath
)

# Простейший парсер-заглушка, создающий корректный заголовочник elxinfo.h
$content = @"
// Auto-generated stub for elxinfo.h
#ifndef ELXINFO_H
#define ELXINFO_H

// Generated from mergeelx.c source payload boundary
#define ELX_STID_MAGIC 0x4F707573

#endif
"@

$outputDir = Split-Path -Parent $OutputPath
if (!(Test-Path $outputDir)) {
    New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
}

Set-Content -Path $OutputPath -Value $content -Encoding UTF8
