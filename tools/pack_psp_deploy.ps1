# Slim PSP deploy: EBOOT + data + single sprites.pak (no loose PNG tree).
# From repo root:
#   powershell -File tools/pack_psp_deploy.ps1

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Psp = Join-Path $Root "cpp\platform\psp"
$Eboot = Join-Path $Psp "EBOOT.PBP"
$DataSrc = Join-Path $Root "cpp\data"
$Out = Join-Path $Psp "deploy\MINIDAYZ"
$Images = Join-Path $Root "original_docs\images"
$PackJs = Join-Path $Root "tools\pack_psp_sprites.js"

if (-not (Test-Path $Eboot)) {
  throw "Missing EBOOT.PBP. Build first in cpp/platform/psp via WSL make"
}

if (Test-Path $Out) { Remove-Item $Out -Recurse -Force }
New-Item -ItemType Directory -Path (Join-Path $Out "data") | Out-Null

Copy-Item $Eboot (Join-Path $Out "EBOOT.PBP")
Copy-Item (Join-Path $DataSrc "*") (Join-Path $Out "data") -Recurse

$UiCandidates = @(
  (Join-Path $Root "original_docs\l_eng_ui.xml"),
  (Join-Path $Root "cpp\docs\l_eng_ui.xml")
)
foreach ($u in $UiCandidates) {
  if (Test-Path $u) {
    Copy-Item $u (Join-Path $Out "data\l_eng_ui.xml")
    break
  }
}

# Original sounds (C2 media/*.ogg) for platform/psp/audio_psp.cpp.
$Media = Join-Path $Root "original_docs\media"
if (-not (Test-Path $Media)) { throw "Missing original_docs/media - cannot deploy sounds" }
New-Item -ItemType Directory -Path (Join-Path $Out "data\media") | Out-Null
Copy-Item (Join-Path $Media "*.ogg") (Join-Path $Out "data\media")

$PakOut = Join-Path $Out "data\sprites.pak"
if (-not (Test-Path $Images)) {
  throw "Missing original_docs/images - cannot pack sprites"
}
$IntroDir = Join-Path $Root "cpp\data\intro"
node $PackJs $Images $PakOut $IntroDir
if (Test-Path (Join-Path $Out "data\intro")) { Remove-Item (Join-Path $Out "data\intro") -Recurse -Force }
if (-not (Test-Path $PakOut)) { throw "sprites.pak not created" }

$files = (Get-ChildItem $Out -Recurse -File).Count
$mb = [math]::Round(((Get-ChildItem $Out -Recurse -File | Measure-Object Length -Sum).Sum) / 1MB, 2)
Write-Host "Deploy ready: $Out"
Write-Host "Files: $files  Size: $mb MB  (sprites in one sprites.pak)"
Write-Host "Copy MINIDAYZ to PSP/GAME/ - do NOT add loose original_docs/images."
