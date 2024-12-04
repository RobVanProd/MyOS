# Download Git for Windows
$gitUrl = "https://github.com/git-for-windows/git/releases/download/v2.43.0.windows.1/Git-2.43.0-64-bit.exe"
$outputPath = "$env:TEMP\GitInstaller.exe"

Write-Host "Downloading Git..."
Invoke-WebRequest -Uri $gitUrl -OutFile $outputPath

Write-Host "Installing Git..."
Start-Process -FilePath $outputPath -ArgumentList "/VERYSILENT /NORESTART" -Wait

Write-Host "Git installation complete!"

# Configure Git
git config --global user.name "Robert Van Arsdale"
git config --global user.email "robert.vanarsdale@gmail.com" 