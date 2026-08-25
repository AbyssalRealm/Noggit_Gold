$ErrorActionPreference = 'Stop'

Write-Host 'Noggit_Gold GitHub Publisher' -ForegroundColor Yellow
Write-Host 'Run this script from the ROOT of the extracted Noggit_Gold Public Test TC1 source tree.'

$required = @('README.md','CMakeLists.txt','COPYING','src')
foreach ($item in $required) {
    if (-not (Test-Path $item)) {
        throw "Missing required project item: $item`nExtract the TC1 package and run this script from that extracted project root."
    }
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw 'Git is not installed or is not available in PATH.'
}

$repo = 'https://github.com/AbyssalRealm/Noggit_Gold.git'

if (-not (Test-Path '.git')) {
    git init
}

git branch -M main

$origin = git remote get-url origin 2>$null
if ($LASTEXITCODE -ne 0) {
    git remote add origin $repo
} elseif ($origin -ne $repo) {
    git remote set-url origin $repo
}

git add -A

$pending = git status --porcelain
if ($pending) {
    git commit -m 'Publish Noggit_Gold public test source TC1'
} else {
    Write-Host 'No uncommitted source changes found.' -ForegroundColor Cyan
}

Write-Host ''
Write-Host 'Publishing the complete source tree to AbyssalRealm/Noggit_Gold...' -ForegroundColor Yellow
Write-Host 'The repo currently only contains the staging docs created through ChatGPT, so this initial full-source push intentionally replaces that temporary history.' -ForegroundColor DarkYellow

git push -u origin main --force

Write-Host ''
Write-Host 'Noggit_Gold source publication complete.' -ForegroundColor Green
Write-Host 'https://github.com/AbyssalRealm/Noggit_Gold'
