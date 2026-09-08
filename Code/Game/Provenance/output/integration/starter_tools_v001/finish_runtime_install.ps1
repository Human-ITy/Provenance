# Run only after authorization to restart this checkout's resource helpers.
$ErrorActionPreference='Stop'
$project=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../../../..'))
$build=Join-Path $project 'Build/x64_Debug'
$processes=@(Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath -and ([IO.Path]::GetDirectoryName($_.ExecutablePath) -eq $build) })
if($processes | Where-Object Name -eq 'EsotericaEditor.exe'){throw 'The editor is running; preserve and close it before restarting its helpers.'}
$servers=@($processes | Where-Object Name -eq 'EsotericaResourceServer.exe')
$helpers=@($processes | Where-Object { $_.Name -in @('EsotericaResourceServer.exe','EsotericaResourceCompiler.exe') })
$restart=@()
foreach($server in $servers){
    $argsText=$server.CommandLine.Trim()
    if($argsText.StartsWith('"')){$argsText=$argsText.Substring($argsText.IndexOf('"',1)+1).Trim()}
    else{$argsText=$argsText.Substring($server.ExecutablePath.Length).Trim()}
    $restart+=@{path=$server.ExecutablePath;arguments=$argsText}
}
try {
    foreach($server in $servers){Stop-Process -Id $server.ProcessId -ErrorAction SilentlyContinue}
    foreach($helper in $helpers | Where-Object Name -eq 'EsotericaResourceCompiler.exe'){
        $current=Get-Process -Id $helper.ProcessId -ErrorAction SilentlyContinue
        if($current -and $current.Path -eq $helper.ExecutablePath){Stop-Process -Id $helper.ProcessId -ErrorAction Stop}
    }
    & 'C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe' (Join-Path $project 'Code/Game/Esoterica.Game.Runtime.vcxproj') /t:Build /m:2 /nologo /verbosity:minimal /p:Configuration=Debug /p:Platform=x64 "/p:SolutionDir=$project\" /p:BuildProjectReferences=false /p:PreBuildEventUseInBuild=false *> (Join-Path $PSScriptRoot 'runtime_install_build.log')
    if($LASTEXITCODE -ne 0){Get-Content (Join-Path $PSScriptRoot 'runtime_install_build.log') -Tail 25;throw 'Runtime installation build failed.'}
} finally {
    foreach($server in $restart){
        $launch=@{FilePath=$server.path;WorkingDirectory=$build;WindowStyle='Hidden'}
        if($server.arguments){$launch.ArgumentList=$server.arguments}
        Start-Process @launch
    }
}
Write-Output 'Corrected runtime installed; project resource server restarted.'
