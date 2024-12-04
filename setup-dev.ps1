# Install Chocolatey if not already installed
if (!(Test-Path "$env:ProgramData\chocolatey\choco.exe")) {
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
}

# Install required packages
choco install mingw -y
choco install nasm -y
choco install make -y
choco install qemu -y

# Add tools to PATH
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine")

# Create i686-elf cross compiler directory
New-Item -ItemType Directory -Force -Path "C:\cross" | Out-Null
$env:PATH += ";C:\cross\bin"

Write-Host "Development environment setup complete!"
Write-Host "Please restart your terminal to use the new tools." 