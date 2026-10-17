Add-Type -AssemblyName System.Drawing
$files = @(
  'tilemap_forest.png',
  'ground_lvl1_tilemap.png',
  'ground_enviroment_tilemap.png',
  'tilemap_sand.png',
  'tilemap_water.png'
)
$dir = 'C:\Users\abdur\Desktop\MiniDayZ\original_docs\images'
foreach ($f in $files) {
  $p = Join-Path $dir $f
  if (Test-Path $p) {
    $i = [System.Drawing.Image]::FromFile($p)
    Write-Output ("{0} {1}x{2}" -f $f, $i.Width, $i.Height)
    $i.Dispose()
  }
}
