Remove-Item -Path "build" -Recurse -ErrorAction SilentlyContinue
Remove-Item -Path "googletest" -Recurse -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path "../lib" -ErrorAction SilentlyContinue | Out-Null

git clone https://github.com/google/googletest.git

New-Item -ItemType Directory -Path "build" | Out-Null
Set-Location "build"

cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ../googletest
cmake --build . --config Release

Copy-Item -Path "lib/*.a" -Destination "../lib/" -ErrorAction SilentlyContinue
Copy-Item -Path "*.a" -Destination "../lib/" -ErrorAction SilentlyContinue

Get-ChildItem -Filter "*.a" -Recurse | ForEach-Object {
    Copy-Item -Path $_.FullName -Destination "../lib/" -Force
}

Set-Location ".."
Write-Host "Finish! Check ../lib directory for files." -ForegroundColor Green