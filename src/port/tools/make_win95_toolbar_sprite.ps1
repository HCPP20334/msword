param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$source = [System.Drawing.Bitmap]::new($InputPath)
$cellSize = 20
$columns = 5
$rows = 4
$sprite = [System.Drawing.Bitmap]::new(
    $cellSize * 17,
    $cellSize,
    [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)

$buttonX = @(197, 436, 675, 911, 1148)
$buttonY = @(68, 304, 538, 777)
$palette = @(
    [System.Drawing.Color]::FromArgb(0, 0, 0),
    [System.Drawing.Color]::FromArgb(128, 128, 128),
    [System.Drawing.Color]::FromArgb(192, 192, 192),
    [System.Drawing.Color]::FromArgb(255, 255, 255),
    [System.Drawing.Color]::FromArgb(0, 0, 128),
    [System.Drawing.Color]::FromArgb(0, 0, 255),
    [System.Drawing.Color]::FromArgb(0, 128, 128),
    [System.Drawing.Color]::FromArgb(0, 255, 255),
    [System.Drawing.Color]::FromArgb(128, 128, 0),
    [System.Drawing.Color]::FromArgb(255, 255, 0),
    [System.Drawing.Color]::FromArgb(128, 0, 0),
    [System.Drawing.Color]::FromArgb(255, 0, 0),
    [System.Drawing.Color]::FromArgb(0, 128, 0),
    [System.Drawing.Color]::FromArgb(0, 255, 0),
    [System.Drawing.Color]::FromArgb(128, 0, 128),
    [System.Drawing.Color]::FromArgb(255, 0, 255)
)

$graphics = [System.Drawing.Graphics]::FromImage($sprite)
$graphics.Clear([System.Drawing.Color]::FromArgb(192, 192, 192))
$graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy

for ($index = 0; $index -lt 17; ++$index) {
    $column = $index % $columns
    $row = [Math]::Floor($index / $columns)
    $sourceRect = [System.Drawing.Rectangle]::new(
        $buttonX[$column] + 20,
        $buttonY[$row] + 20,
        152,
        152)
    $destinationRect = [System.Drawing.Rectangle]::new(
        $index * $cellSize,
        0,
        $cellSize,
        $cellSize)
    $graphics.DrawImage(
        $source,
        $destinationRect,
        $sourceRect,
        [System.Drawing.GraphicsUnit]::Pixel)
}

$graphics.Dispose()
$source.Dispose()

for ($y = 0; $y -lt $sprite.Height; ++$y) {
    for ($x = 0; $x -lt $sprite.Width; ++$x) {
        $pixel = $sprite.GetPixel($x, $y)
        $best = $palette[0]
        $bestDistance = [int]::MaxValue
        foreach ($candidate in $palette) {
            $dr = [int]$pixel.R - [int]$candidate.R
            $dg = [int]$pixel.G - [int]$candidate.G
            $db = [int]$pixel.B - [int]$candidate.B
            $distance = $dr * $dr + $dg * $dg + $db * $db
            if ($distance -lt $bestDistance) {
                $bestDistance = $distance
                $best = $candidate
            }
        }
        $sprite.SetPixel($x, $y, $best)
    }
}

$outputDirectory = Split-Path -Parent $OutputPath
if ($outputDirectory) {
    [System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
}
$sprite.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Bmp)
$sprite.Dispose()
