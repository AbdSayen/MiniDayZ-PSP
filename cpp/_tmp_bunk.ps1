Add-Type -AssemblyName System.Drawing
foreach ($f in @('tilemap_bunker.png','ground_lvl1_tilemap.png')) {
  $p = Join-Path 'C:\Users\abdur\Desktop\MiniDayZ\original_docs\images' $f
  $i = [System.Drawing.Image]::FromFile($p)
  Write-Output ("{0} {1}x{2}" -f $f, $i.Width, $i.Height)
  $i.Dispose()
}
