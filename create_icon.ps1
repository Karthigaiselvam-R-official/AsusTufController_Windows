
$source = "ui/app_icon.png"
$dest = "resources/app_icon.ico"

Add-Type -AssemblyName System.Drawing
$bmp = [System.Drawing.Bitmap]::FromFile($source)
$icon = [System.Drawing.Icon]::FromHandle($bmp.GetHicon())
$file = [System.IO.File]::Open($dest, "OpenOrCreate")
$icon.Save($file)
$file.Close()
$icon.Dispose()
$bmp.Dispose()
Write-Host "Icon created at $dest"
