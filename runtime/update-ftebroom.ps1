param([Parameter(Mandatory=$true)][string]$Root,[Parameter(Mandatory=$true)][string]$CurrentVersion,[int]$ParentId=0,[string]$LocalPackage='')
$ErrorActionPreference='Stop'
$Root=[IO.Path]::GetFullPath($Root)
if(!(Test-Path -LiteralPath "$Root/fteqw64.exe") -and !(Test-Path -LiteralPath "$Root/bin/fteqw64.exe")){throw 'Invalid editor installation'}
$updates=Join-Path $Root 'updates'
New-Item -ItemType Directory -Force -Path $updates | Out-Null
function Status([string]$text){[IO.File]::WriteAllText("$updates/status.txt",$text)}
function Under([string]$base,[string]$relative){$p=[IO.Path]::GetFullPath((Join-Path $base $relative));if(!$p.StartsWith($base.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Unsafe archive path'};return $p}
function FileHash([string]$path){$stream=[IO.File]::OpenRead($path);$sha=[Security.Cryptography.SHA256]::Create();try{return [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLowerInvariant()}finally{$stream.Dispose();$sha.Dispose()}}
function CleanupUpdates([string]$keep){
 $base=[IO.Path]::GetFullPath($updates)
 if((Get-Item -LiteralPath $base).Attributes -band [IO.FileAttributes]::ReparsePoint){return}
 foreach($item in Get-ChildItem -LiteralPath $base){
  $target=Under $base $item.Name
  if($target -eq $keep -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)){continue}
  if($item.PSIsContainer -and $item.Name -match '^(stage|backup)-[a-zA-Z0-9_-]+$'){
   # Avoid traversing junctions, including those inside an old staging folder.
   $pending=[Collections.Generic.Queue[string]]::new();$pending.Enqueue($target);$linked=$false
   while($pending.Count -and !$linked){foreach($child in Get-ChildItem -LiteralPath $pending.Dequeue() -Force){
    if($child.Attributes -band [IO.FileAttributes]::ReparsePoint){$linked=$true;break}
    if($child.PSIsContainer){$pending.Enqueue($child.FullName)}
   }}
   if(!$linked){Remove-Item -LiteralPath $target -Recurse -Force}
  }elseif(!$item.PSIsContainer -and $item.Name -match '^(?:ReyondBroom|FTEBroom)-[0-9][a-zA-Z0-9.\-]*-(Windows-x64\.zip|Linux-x64\.tar\.gz)(\.part|\.sha256)?$'){
   Remove-Item -LiteralPath $target -Force
  }
 }
}
function VersionKey([string]$tag){if($tag -notmatch '^v?(\d+)\.(\d+)\.(\d+)(?:-alpha\.(\d+))?$'){return $null};$alpha=if($Matches[4]){[int]$Matches[4]}else{999999};return [version]("{0}.{1}.{2}.{3}" -f $Matches[1],$Matches[2],$Matches[3],$alpha)}
$oldManifest=@{}
if(Test-Path -LiteralPath "$Root/distribution-manifest.json"){try{$manifest=Get-Content -LiteralPath "$Root/distribution-manifest.json" -Raw | ConvertFrom-Json;foreach($property in $manifest.PSObject.Properties){$oldManifest[$property.Name]=$property.Value}}catch{}}
function Preserve([string]$name){
 $p=$name.Replace('\','/');$leaf=[IO.Path]::GetFileName($p)
 $existing=Under $Root $p
 if((Test-Path -LiteralPath $existing) -and $p -match '^ftebroom/.*\.(cfg|ini|map|wad|def|ent|fgd)$'){
  if(!$oldManifest.ContainsKey($p)){return $true}
  if((FileHash $existing) -ne $oldManifest[$p]){return $true}
 }
 return ($p -match '^(downloads|updates|ftebroom/(autosave|cache))/') -or ($leaf -match '^(ftebroom-(project|game-profile|quake1|build|preferences|run|plugins|camera).*\.(txt|cfg)|appearance\.cfg|workspace\.ini|config\.cfg|fte\.cfg|installed\.lst)$')
}
$mutex=$null;$stage=$null;$backup=$null;$changed=@();$process=$null
try {
 $hash=[BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($Root.ToLowerInvariant()))).Replace('-','')
 $mutex=New-Object Threading.Mutex($false,('Local\FTEBroomUpdate'+$hash))
 if(!$mutex.WaitOne(0)){exit 0}
 if($ParentId){$process=[Diagnostics.Process]::GetProcessById($ParentId)}
 Status 'Checking GitHub releases...'
 if(!$LocalPackage){
  [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
  $releases=Invoke-RestMethod -Uri 'https://api.github.com/repos/reyond9079/ReyondBroom/releases?per_page=100' -Headers @{'User-Agent'='ReyondBroom-Updater'} -TimeoutSec 30
  $current=VersionKey $CurrentVersion
  $release=$releases | Where-Object {
   $candidate="ReyondBroom-$($_.tag_name.TrimStart('v'))-Windows-x64.zip"
   !$_.draft -and (VersionKey $_.tag_name) -and (VersionKey $_.tag_name) -gt $current -and ($_.assets.name -contains $candidate) -and ($_.assets.name -contains "$candidate.sha256")
  } | Sort-Object @{Expression={VersionKey $_.tag_name};Descending=$true} | Select-Object -First 1
  if(!$release){Status 'ReyondBroom is up to date.';exit 0}
  $version=$release.tag_name.TrimStart('v');$name="ReyondBroom-$version-Windows-x64.zip"
  $asset=$release.assets | Where-Object name -eq $name | Select-Object -First 1
  $sum=$release.assets | Where-Object name -eq "$name.sha256" | Select-Object -First 1
  if(!$asset -or !$sum){throw 'The newer release has no complete Windows package yet.'}
  foreach($a in @($asset,$sum)){if(!$a.browser_download_url.StartsWith('https://github.com/reyond9079/ReyondBroom/releases/download/')){throw 'Unexpected download URL'}}
  $checksumResponse=(Invoke-WebRequest -UseBasicParsing $sum.browser_download_url -TimeoutSec 30).Content
  if($checksumResponse -is [byte[]]){$checksumResponse=[Text.Encoding]::UTF8.GetString($checksumResponse)}
  $checksum=([string]$checksumResponse).Trim().Split(' ')[0].ToLowerInvariant()
  if($checksum -notmatch '^[a-f0-9]{64}$'){throw 'Invalid release checksum'}
  $LocalPackage=Join-Path $updates $name
  $request=[Net.HttpWebRequest]::Create($asset.browser_download_url);$request.UserAgent='ReyondBroom-Updater';$request.Timeout=30000;$response=$request.GetResponse()
  try{$inputStream=$response.GetResponseStream();$output=[IO.File]::Create($LocalPackage+'.part');try{$buffer=New-Object byte[] 65536;$received=0L;$last=[DateTime]::MinValue;while(($n=$inputStream.Read($buffer,0,$buffer.Length)) -gt 0){$received+=$n;if($received -gt 536870912){throw 'Update exceeds 512 MiB'};$output.Write($buffer,0,$n);if(([DateTime]::Now-$last).TotalMilliseconds -gt 200){Status ("Downloading {0}: {1:N1} / {2:N1} MiB" -f $version,($received/1MB),($asset.size/1MB));$last=[DateTime]::Now}}}finally{$output.Dispose();$inputStream.Dispose()}}finally{$response.Dispose()}
  if((FileHash ($LocalPackage+'.part')) -ne $checksum){throw 'Update checksum mismatch'}
  Move-Item -LiteralPath ($LocalPackage+'.part') -Destination $LocalPackage -Force
 }
 Add-Type -AssemblyName System.IO.Compression.FileSystem
 $stage=Join-Path $updates ('stage-'+[guid]::NewGuid());New-Item -ItemType Directory -Path $stage | Out-Null
 $zip=[IO.Compression.ZipFile]::OpenRead($LocalPackage)
 try {$total=0L;foreach($entry in $zip.Entries){$name=$entry.FullName.Replace('\','/');if(!$name.StartsWith('ReyondBroom-Windows-x64/')){throw 'Unexpected package layout'};$relative=$name.Substring('ReyondBroom-Windows-x64/'.Length);if(!$relative){continue};$target=Under $stage $relative;$total+=$entry.Length;if($total -gt 2147483648){throw 'Expanded update exceeds 2 GiB'};if($name.EndsWith('/')){New-Item -ItemType Directory -Force $target | Out-Null}else{New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($target)) | Out-Null;[IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$target)}}}finally{$zip.Dispose()}
 if((!(Test-Path "$stage/fteqw64.exe") -and !(Test-Path "$stage/bin/fteqw64.exe")) -or !(Test-Path "$stage/fteplug_ftebroom_x64.dll")){throw 'Incomplete update'}
 Status 'Update downloaded. It will be installed after you close the editor.'
 if($process){$process.WaitForExit()}
 $backup=Join-Path $updates ('backup-'+[guid]::NewGuid());New-Item -ItemType Directory -Path $backup | Out-Null
 foreach($file in Get-ChildItem -LiteralPath $stage -File -Recurse){
  $relative=$file.FullName.Substring($stage.Length+1);if(Preserve $relative){continue};$target=Under $Root $relative;$saved=Under $backup $relative
  $existed=Test-Path -LiteralPath $target;if($existed){New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($saved)) | Out-Null;Copy-Item -LiteralPath $target -Destination $saved}
  $changed+=,@($target,$saved,$existed);New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($target)) | Out-Null
  $temporary=$target+'.ftebroom-update.tmp';Copy-Item -LiteralPath $file.FullName -Destination $temporary -Force;Move-Item -LiteralPath $temporary -Destination $target -Force
 }
 if((Test-Path "$stage/bin/fteqw64.exe") -and (Test-Path "$Root/fteqw64.exe")){
  $target=Under $Root 'fteqw64.exe';$saved=Under $backup 'fteqw64.exe'
  Copy-Item -LiteralPath $target -Destination $saved -Force
  $changed+=,@($target,$saved,$true);Remove-Item -LiteralPath $target
 }
 try{CleanupUpdates $backup}catch{} # Cache cleanup must not roll back a successful install.
 Status 'Update installed. Your settings and documents were preserved. Start ReyondBroom normally.'
} catch {
 foreach($item in $changed){try{if($item[2]){Copy-Item -LiteralPath $item[1] -Destination $item[0] -Force}else{Remove-Item -LiteralPath $item[0] -Force}}catch{}}
 Status ('Update failed: '+$_.Exception.Message);exit 1
} finally {
 if($process){$process.Dispose()}
 if($mutex){try{$mutex.ReleaseMutex()}catch{};$mutex.Dispose()}
 # Failed updates retain staging and rollback files for diagnosis.
}
