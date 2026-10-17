Add-Type -AssemblyName System.Drawing
$dir = 'C:\Users\abdur\Desktop\MiniDayZ\original_docs\images'
$files = @(
  'gui_portrait_bg-sheet0.png','gui_panel-sheet0.png','gui_blood_bar-sheet0.png',
  'gui_slot_bg-sheet0.png','gui_time_bg-sheet0.png','minimap_tile-sheet0.png',
  'gui_btn_inventory-sheet0.png','gui_btn_attack-sheet0.png','gui_dpad_field-sheet0.png'
)
foreach ($f in $files) {
  $p = Join-Path $dir $f
  if (Test-Path $p) {
    $i = [System.Drawing.Image]::FromFile($p)
    Write-Output ("{0} {1}x{2}" -f $f, $i.Width, $i.Height)
    $i.Dispose()
  } else {
    Write-Output ("{0} MISS" -f $f)
  }
}
