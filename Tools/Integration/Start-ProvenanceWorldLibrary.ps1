[CmdletBinding()]
param(
    [string] $CanonicalWorkspaceRoot = '',
    [switch] $ValidateOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$clientRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$playScript = Join-Path $PSScriptRoot 'Start-Ei3CanonicalPlayable.ps1'
$libraryRoot = Join-Path $clientRoot 'Data\Worlds'
$libraryPath = Join-Path $libraryRoot 'player-worlds.json'
$macroRoot = Join-Path $clientRoot 'Data\Worldgen\MacroAuthority-v11'
if (-not $CanonicalWorkspaceRoot) {
    $candidate = Resolve-Path -LiteralPath (Join-Path $clientRoot '..\.ei3qb-cert-workspace') -ErrorAction SilentlyContinue
    if ($candidate) { $CanonicalWorkspaceRoot = $candidate.Path }
}
if (-not $CanonicalWorkspaceRoot -or -not (Test-Path -LiteralPath $CanonicalWorkspaceRoot -PathType Container)) {
    throw 'The canonical FableScript workspace could not be located.'
}
$worldStateRoot = Join-Path $CanonicalWorkspaceRoot 'State\canonical-playable'
New-Item -ItemType Directory -Force -Path $libraryRoot | Out-Null

function New-WorldRecord([string] $Name,[string] $Seed,[string] $TerrainLaw='orographic.phase17') {
    [pscustomobject]@{ id=[guid]::NewGuid().ToString('N'); name=$Name; seed=$Seed
        terrain_law=$TerrainLaw
        created_utc=[DateTime]::UtcNow.ToString('o'); last_played_utc=$null }
}
function Resolve-TerrainLaw($World) {
    $law = ''
    if ($World.PSObject.Properties['terrain_law']) { $law = [string]$World.terrain_law }
    if ($law) { return $law }
    $seed = [string]$World.seed
    if ($seed -eq '20260827' -or $seed -eq 'orographic-phase17-canonical') {
        return 'orographic.phase17'
    }
    if ($seed -like 'provenance-*') { return 'worldgenesis.v11' }
    return 'orographic.phase17'
}
function Load-Library {
    if (Test-Path -LiteralPath $libraryPath -PathType Leaf) {
        try {
            $j=Get-Content -LiteralPath $libraryPath -Raw|ConvertFrom-Json
            return [pscustomobject]@{ schema='provenance.player-world-library/1'
                last_world_id=[string]$j.last_world_id
                compass=if($null-ne $j.compass){[bool]$j.compass}else{$true}
                human_ruler=if($null-ne $j.human_ruler){[bool]$j.human_ruler}else{$true}
                meter_ruler=if($null-ne $j.meter_ruler){[bool]$j.meter_ruler}else{$true}
                worlds=[Collections.ArrayList]@($j.worlds) }
        } catch {}
    }
    $first=New-WorldRecord 'Stage 0 Orographic' '20260827'
    [pscustomobject]@{schema='provenance.player-world-library/1';last_world_id=$first.id
        compass=$true;human_ruler=$true;meter_ruler=$true
        worlds=[Collections.ArrayList]@($first)}
}
function Save-Library {$library|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $libraryPath -Encoding utf8}
function Get-SeedHash([string]$Seed) {
    $h=[Security.Cryptography.SHA256]::Create()
    try{$d=$h.ComputeHash([Text.Encoding]::UTF8.GetBytes($Seed))}finally{$h.Dispose()}
    (-join($d|ForEach-Object{$_.ToString('x2')})).Substring(0,16)
}
function Remove-WorldData($World) {
    $hash=Get-SeedHash ([string]$World.seed)
    $base=Join-Path $worldStateRoot "world-instance-stage0-genesis-v11-$hash.json"
    $oro=Join-Path $worldStateRoot "world-instance-stage0-orographic-phase17-$hash.json"
    $opening="$base.opening.json"
    $origin=Join-Path $worldStateRoot "world-instance-stage0-genesis-v11-$hash-origin.json"
    $oroOrigin=Join-Path $worldStateRoot "world-instance-stage0-orographic-phase17-$hash-origin.json"
    $digest=''
    foreach($receipt in $opening,$origin){
        if(Test-Path -LiteralPath $receipt -PathType Leaf){try{
            $j=Get-Content -LiteralPath $receipt -Raw|ConvertFrom-Json
            if($j.macro_genesis_digest){$digest=[string]$j.macro_genesis_digest}
            elseif($j.genesis_digest){$digest=[string]$j.genesis_digest}
        }catch{}}
    }
    foreach($target in $base,$opening,$origin,$oro,$oroOrigin){if(Test-Path -LiteralPath $target -PathType Leaf){Remove-Item -LiteralPath $target -Force}}
    if($digest-match'^[0-9a-f]{64}$'){$cache=Join-Path $macroRoot $digest
        if(Test-Path -LiteralPath $cache -PathType Container){Remove-Item -LiteralPath $cache -Recurse -Force}}
}

$library=Load-Library
foreach($w in $library.worlds){
    $resolved=Resolve-TerrainLaw $w
    if(-not $w.PSObject.Properties['terrain_law'] -or [string]$w.terrain_law -ne $resolved){
        $w | Add-Member -NotePropertyName terrain_law -NotePropertyValue $resolved -Force
    }
}
$hasCanonical=$false
foreach($w in $library.worlds){
    if([string]$w.name -eq 'Stage 0 Orographic' -or [string]$w.seed -eq 'orographic-phase17-canonical' -or [string]$w.seed -eq '20260827'){ $hasCanonical=$true; break }
}
if(-not $hasCanonical){
    $oro=New-WorldRecord 'Stage 0 Orographic' '20260827'
    [void]$library.worlds.Insert(0, $oro)
}
Save-Library
if ($ValidateOnly) {
    [pscustomobject]@{
        status = 'PASS'
        library_path = $libraryPath
        world_count = $library.worlds.Count
        last_world_id = $library.last_world_id
        compass = $library.compass
        human_ruler = $library.human_ruler
        meter_ruler = $library.meter_ruler
    } | ConvertTo-Json -Compress
    exit 0
}
$form=New-Object Windows.Forms.Form
$form.Text='Provenance - Worlds';$form.StartPosition='CenterScreen'
$form.ClientSize=New-Object Drawing.Size(820,560)
$form.BackColor=[Drawing.Color]::FromArgb(24,29,28);$form.ForeColor=[Drawing.Color]::FromArgb(236,228,190)
$form.Font=New-Object Drawing.Font('Segoe UI',10);$form.FormBorderStyle='FixedDialog';$form.MaximizeBox=$false
function Add-Label([string]$text,[int]$x,[int]$y,[int]$w,[int]$h,[int]$size=10){
    $c=New-Object Windows.Forms.Label;$c.Text=$text;$c.Location=New-Object Drawing.Point($x,$y)
    $c.Size=New-Object Drawing.Size($w,$h);if($size-ne 10){$c.Font=New-Object Drawing.Font('Segoe UI Semibold',$size)}
    $form.Controls.Add($c);$c
}
[void](Add-Label 'PROVENANCE WORLDS' 28 20 500 42 20)
$sub=Add-Label 'Seed-deterministic worlds resume exactly as themselves.' 31 61 620 24
$sub.ForeColor=[Drawing.Color]::FromArgb(165,174,158)
$list=New-Object Windows.Forms.ListBox;$list.Location=New-Object Drawing.Point(28,100)
$list.Size=New-Object Drawing.Size(350,350);$list.BackColor=[Drawing.Color]::FromArgb(15,19,20)
$list.ForeColor=[Drawing.Color]::FromArgb(235,228,195);$form.Controls.Add($list)
[void](Add-Label 'World name' 414 104 180 22)
$nameBox=New-Object Windows.Forms.TextBox;$nameBox.Location=New-Object Drawing.Point(414,129)
$nameBox.Size=New-Object Drawing.Size(370,28);$form.Controls.Add($nameBox)
[void](Add-Label 'World seed' 414 176 180 22)
$seedBox=New-Object Windows.Forms.TextBox;$seedBox.Location=New-Object Drawing.Point(414,201)
$seedBox.Size=New-Object Drawing.Size(370,28);$form.Controls.Add($seedBox)
function Add-Check([string]$text,[int]$y,[bool]$checked){$c=New-Object Windows.Forms.CheckBox
    $c.Text=$text;$c.Location=New-Object Drawing.Point(414,$y);$c.Size=New-Object Drawing.Size(320,26)
    $c.Checked=$checked;$form.Controls.Add($c);$c}
$compass=Add-Check 'Show compass' 254 $library.compass
$human=Add-Check 'Show 6 ft human scale ruler' 286 $library.human_ruler
$meter=Add-Check 'Show infinite metre ruler' 318 $library.meter_ruler
function Add-Button([string]$text,[int]$x,[int]$y,[int]$w){$b=New-Object Windows.Forms.Button
    $b.Text=$text;$b.Location=New-Object Drawing.Point($x,$y);$b.Size=New-Object Drawing.Size($w,38)
    $b.FlatStyle='Flat';$b.BackColor=[Drawing.Color]::FromArgb(62,72,57)
    $b.ForeColor=[Drawing.Color]::FromArgb(244,233,182);$form.Controls.Add($b);$b}
$newButton=Add-Button 'NEW WORLD' 28 470 166;$saveButton=Add-Button 'SAVE WORLD' 210 470 168
$deleteButton=Add-Button 'DELETE WORLD' 414 382 176;$playButton=Add-Button 'CONTINUE / PLAY' 608 382 176
$playButton.BackColor=[Drawing.Color]::FromArgb(91,99,55)
function Selected-World {if($list.SelectedIndex-lt 0-or$list.SelectedIndex-ge$library.worlds.Count){return $null};$library.worlds[$list.SelectedIndex]}
function Refresh-WorldList([string]$SelectId=''){
    $list.Items.Clear();$selected=-1
    for($i=0;$i-lt$library.worlds.Count;$i++){$w=$library.worlds[$i]
        [void]$list.Items.Add(('{0}    [{1}]'-f$w.name,$w.seed));if([string]$w.id-eq$SelectId){$selected=$i}}
    if($selected-lt 0-and$library.worlds.Count-gt 0){$selected=0};if($selected-ge 0){$list.SelectedIndex=$selected}}
$list.Add_SelectedIndexChanged({$w=Selected-World;if($w){$nameBox.Text=[string]$w.name;$seedBox.Text=[string]$w.seed}})
$newButton.Add_Click({$list.ClearSelected();$nameBox.Text='Stage 0 Orographic';$seedBox.Text='20260827';$nameBox.SelectAll();$nameBox.Focus()})
$saveButton.Add_Click({$name=$nameBox.Text.Trim();$seed=$seedBox.Text.Trim()
    if(-not$name-or-not$seed){[void][Windows.Forms.MessageBox]::Show('A world name and seed are required.','Provenance');return}
    $w=Selected-World
    if($w){$w.name=$name;$w.seed=$seed
        if(-not $w.PSObject.Properties['terrain_law'] -or -not [string]$w.terrain_law){
            $w | Add-Member -NotePropertyName terrain_law -NotePropertyValue (Resolve-TerrainLaw $w) -Force
        }
    }else{$w=New-WorldRecord $name $seed 'orographic.phase17';[void]$library.worlds.Add($w)}
    Save-Library;Refresh-WorldList ([string]$w.id)})
$deleteButton.Add_Click({$w=Selected-World;if(-not$w){return}
    $a=[Windows.Forms.MessageBox]::Show("Delete '$($w.name)'?`r`n`r`nIts saved identity and generated terrain cache will be removed.",'Delete world','YesNo','Warning')
    if($a-ne[Windows.Forms.DialogResult]::Yes){return};Remove-WorldData $w;$library.worlds.RemoveAt($list.SelectedIndex)
    if([string]$library.last_world_id-eq[string]$w.id){$library.last_world_id=''};Save-Library;Refresh-WorldList})
$playAction={$w=Selected-World;if(-not$w){[void][Windows.Forms.MessageBox]::Show('Create or select a world first.','Provenance');return}
    $library.compass=$compass.Checked;$library.human_ruler=$human.Checked;$library.meter_ruler=$meter.Checked
    $library.last_world_id=[string]$w.id;$w.last_played_utc=[DateTime]::UtcNow.ToString('o');Save-Library
    $script:selectedWorld=$w;$form.DialogResult='OK';$form.Close()}
$playButton.Add_Click($playAction);$list.Add_DoubleClick($playAction)
Refresh-WorldList ([string]$library.last_world_id);$selectedWorld=$null
[void]$form.ShowDialog();if(-not$selectedWorld){exit 0}
$clientArguments=@('--ei3-authority');if(-not$library.compass){$clientArguments+='--player-hide-compass'}
if(-not$library.human_ruler){$clientArguments+='--player-hide-human-ruler'};if(-not$library.meter_ruler){$clientArguments+='--player-hide-meter-ruler'}
$playLaw=Resolve-TerrainLaw $selectedWorld
& $playScript -PlayerFacing -RichLandforms -WorldSeed ([string]$selectedWorld.seed) -TerrainLaw $playLaw -CanonicalWorkspaceRoot $CanonicalWorkspaceRoot -ClientArgument $clientArguments
exit $LASTEXITCODE
