# One-Click Update Automation
# This script will send the update.json change to GitHub.

# Use system git
$git = "git"

Write-Host ">>> Starting Update Push..." -ForegroundColor Cyan

# Check if git exists
try {
    & $git --version | Out-Null
}
catch {
    Write-Host "ERROR: Could not find 'git' in PATH." -ForegroundColor Red
    exit
}

# Check if .git exists, if not, initialize
if (-not (Test-Path ".git")) {
    Write-Host "This folder is not a Git repo. Initializing..." -ForegroundColor Yellow
    & $git init
    & $git remote add origin "https://github.com/SKYJ0/GOLEVENTS_PRO.git"
    & $git fetch
    & $git branch -M main
    # Try to pull to resolve history if needed, or just force push if user is owner
    # But safe start: just init.
}

# 1. Add changes
& $git add update.json

# 2. Commit
& $git commit -m "Update v2.3.3: UI Improvements & Import Features"

# 3. Push
Write-Host "Uploading to GitHub..."
& $git push -u origin main

Write-Host "✅ DONE! The update is live." -ForegroundColor Green
Read-Host "Press Enter to exit..."
