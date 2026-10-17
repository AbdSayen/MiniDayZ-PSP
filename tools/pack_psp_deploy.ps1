# Build a slim PSP deploy folder: EBOOT + data only (NO original_docs images/media).
# Usage (from repo root):
#   powershell -File tools/pack_psp_deploy.ps1
#
# Output: cpp/platform/psp/deploy/MINIDAYZ/
# Copy that folder to ms0:/PSP/GAME/ or PPSSPP memstick/PSP/GAME/

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Psp = Join-Path $Root "cpp\platform\psp"
$Eboot = Join-Path $Psp "EBOOT.PBP"
$DataSrc = Join-Path $Root "cpp\data"
$Out = Join-Path $Psp "deploy\MINIDAYZ"

if (-not (Test-Path $Eboot)) {
  throw "Missing EBOOT.PBP. Build first: wsl make in cpp/platform/psp"
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

$files = (Get-ChildItem $Out -Recurse -File).Count
$mb = [math]::Round(((Get-ChildItem $Out -Recurse -File | Measure-Object Length -Sum).Sum) / 1MB, 2)
Write-Host "Deploy ready: $Out"
Write-Host "Files: $files  Size: $mb MB"
Write-Host "Copy MINIDAYZ to PSP/GAME/ - do NOT add original_docs/images next to EBOOT."
