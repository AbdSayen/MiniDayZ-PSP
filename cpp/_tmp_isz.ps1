Add-Type -AssemblyName System.Drawing
$dir = 'C:\Users\abdur\Desktop\MiniDayZ\original_docs\images'
foreach ($f in @(
  'fnx_pistol-sheet0.png','akm_rifle-sheet0.png','bandage-sheet0.png',
  'canned_beans-sheet0.png','zed_bullet-sheet0.png','spawn_loot_point-sheet0.png',
  'gui_btn_attack-sheet0.png','gui_btn_interact-sheet0.png','gui_dpad_field-sheet0.png',
  'gui_dpad_stick-sheet0.png','gui_btn_reload-sheet0.png','ground_enviroment_tilemap.png',
  'knife_hunter-sheet0.png','shovel-sheet0.png','ammo_9mm-sheet0.png'
)) {
  $p = Join-Path $dir $f
  if (Test-Path $p) {
    $i = [System.Drawing.Image]::FromFile($p)
    Write-Output ("{0} {1}x{2}" -f $f, $i.Width, $i.Height)
    $i.Dispose()
  }
}
