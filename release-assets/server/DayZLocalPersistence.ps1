param(
    [Parameter(Mandatory=$true)][string]$Root
)

$ErrorActionPreference = 'Stop'
$Root = [IO.Path]::GetFullPath($Root)
$profileDir = Join-Path $Root '!ConfigServer'
$playerDir = Join-Path $profileDir 'PlayerSaves'
$aliveDir = Join-Path $playerDir 'Alive'
$deadDir = Join-Path $playerDir 'Dead'
$logsDir = Join-Path $profileDir 'Logs'
$storageDir = Join-Path $Root 'storage_1\dayz_Auto.ChernarusPlus\000'
$worldFile = Join-Path $storageDir 'world-items.json'
$logFile = Join-Path $logsDir 'LocalPersistence.log'
$readyFile = Join-Path $logsDir 'LocalPersistence.ready'
$port = 28100

foreach ($dir in @($aliveDir, $deadDir, $logsDir, $storageDir)) {
    [IO.Directory]::CreateDirectory($dir) | Out-Null
}

function Write-StoreLog([string]$message) {
    $line = '[{0}] {1}{2}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $message, [Environment]::NewLine
    [IO.File]::AppendAllText($logFile, $line, [Text.Encoding]::UTF8)
}

function Send-Response($context, [int]$status, [string]$body) {
    $bytes = [Text.Encoding]::UTF8.GetBytes($body)
    $context.Response.StatusCode = $status
    $context.Response.ContentType = 'application/json; charset=utf-8'
    $context.Response.ContentLength64 = $bytes.Length
    $context.Response.OutputStream.Write($bytes, 0, $bytes.Length)
    $context.Response.Close()
}

function Get-RequestBody($request) {
    $reader = [IO.StreamReader]::new($request.InputStream, $request.ContentEncoding)
    try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
}

function Get-PlayerPath([string]$uid, [string]$state = 'Alive') {
    if ([string]::IsNullOrWhiteSpace($uid)) { return $null }
    $safe = [regex]::Replace($uid, '[^A-Za-z0-9_-]', '_')
    $directory = if ($state -eq 'Dead') { $deadDir } else { $aliveDir }
    return (Join-Path $directory ($safe + '.json'))
}

function Test-PlayerAlive($record) {
    if ($null -eq $record -or $null -eq $record.PSObject.Properties['alive']) { return $true }
    try { return ([double]$record.alive -gt 0) } catch { return [bool]$record.alive }
}

function Read-PlayerRecord([string]$uid) {
    $alivePath = Get-PlayerPath $uid 'Alive'
    $deadPath = Get-PlayerPath $uid 'Dead'
    $legacyPath = Join-Path $playerDir (([regex]::Replace($uid, '[^A-Za-z0-9_-]', '_')) + '.json')

    $record = Read-JsonFile $alivePath
    if ($null -ne $record) { return $record }

    # Migrate saves made by the previous flat PlayerSaves layout on first access.
    $record = Read-JsonFile $legacyPath
    if ($null -ne $record) {
        $target = if (Test-PlayerAlive $record) { $alivePath } else { $deadPath }
        Move-Item -LiteralPath $legacyPath -Destination $target -Force
        return $record
    }

    $record = Read-JsonFile $deadPath
    return $record
}

function Write-PlayerRecord([string]$uid, $record) {
    $alivePath = Get-PlayerPath $uid 'Alive'
    $deadPath = Get-PlayerPath $uid 'Dead'
    $legacyPath = Join-Path $playerDir (([regex]::Replace($uid, '[^A-Za-z0-9_-]', '_')) + '.json')
    $target = if (Test-PlayerAlive $record) { $alivePath } else { $deadPath }
    Write-JsonFile $target $record
    # Keep the last death record while a new living character is active.
    if ($target -eq $deadPath -and (Test-Path -LiteralPath $alivePath -PathType Leaf)) {
        Remove-Item -LiteralPath $alivePath -Force
    }
    if (Test-Path -LiteralPath $legacyPath -PathType Leaf) { Remove-Item -LiteralPath $legacyPath -Force }
}

function Read-JsonFile([string]$path) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { return $null }
    $text = [IO.File]::ReadAllText($path, [Text.Encoding]::UTF8)
    if ([string]::IsNullOrWhiteSpace($text)) { return $null }
    return ($text | ConvertFrom-Json)
}

function Write-JsonFile([string]$path, $value) {
    $json = ConvertTo-Json -InputObject $value -Depth 100 -Compress
    $temp = $path + '.tmp'
    [IO.File]::WriteAllText($temp, $json, [Text.Encoding]::UTF8)
    Move-Item -LiteralPath $temp -Destination $path -Force
}

function Read-WorldItems {
    if (-not (Test-Path -LiteralPath $worldFile -PathType Leaf)) { return @() }
    $text = [IO.File]::ReadAllText($worldFile, [Text.Encoding]::UTF8)
    if ([string]::IsNullOrWhiteSpace($text)) { return @() }
    $items = $text | ConvertFrom-Json
    if ($null -eq $items) { return @() }
    return @($items | Where-Object { $null -ne $_ })
}

function Save-WorldItems($items) {
    Write-JsonFile $worldFile @($items)
}

function Handle-Character($context, [string]$route) {
    $request = $context.Request
    $uid = $request.QueryString['uid']
    if ([string]::IsNullOrWhiteSpace($uid)) { Send-Response $context 400 '{"error":"missing uid"}'; return }

    switch ($route) {
        'lud0' {
            if ($request.HttpMethod -eq 'POST') {
                $body = Get-RequestBody $request
                $record = $body | ConvertFrom-Json
                Write-PlayerRecord $uid $record
                Send-Response $context 200 '{"ok":true}'
            } else {
                $saved = Read-PlayerRecord $uid
                if ($null -eq $saved) { Send-Response $context 404 '{"error":"character not found"}'; return }
                Send-Response $context 200 (ConvertTo-Json -InputObject $saved -Depth 100 -Compress)
            }
        }
        'lud0/save' {
            $record = (Get-RequestBody $request) | ConvertFrom-Json
            Write-PlayerRecord $uid $record
            Send-Response $context 200 '{"ok":true}'
        }
        'lud0/load' {
            $saved = Read-PlayerRecord $uid
            if ($null -eq $saved -or -not (Test-PlayerAlive $saved)) { Send-Response $context 404 '{"error":"living character not found"}'; return }
            Send-Response $context 200 (ConvertTo-Json -InputObject $saved -Depth 100 -Compress)
        }
        'lud0/create' {
            $null = Read-PlayerRecord $uid
            $alivePath = Get-PlayerPath $uid 'Alive'
            if (-not (Test-Path -LiteralPath $alivePath)) {
                $initial = [ordered]@{ alive = 1; queue = 0 }
                Write-PlayerRecord $uid $initial
            }
            Send-Response $context 200 '{"ok":true}'
        }
        'lud0/kill' {
            $saved = Read-PlayerRecord $uid
            if ($null -ne $saved) {
                $saved | Add-Member -NotePropertyName alive -NotePropertyValue 0 -Force
                Write-PlayerRecord $uid $saved
            }
            Send-Response $context 200 '{"ok":true}'
        }
        'lud0/find' {
            $saved = Read-PlayerRecord $uid
            if ($null -eq $saved -or -not (Test-PlayerAlive $saved) -or -not $saved.model) { Send-Response $context 404 '{"error":"living character not found"}'; return }
            if (-not $saved.pos) { $saved | Add-Member -NotePropertyName pos -NotePropertyValue @(7500,7500,0) -Force }
            if (-not $saved.queue) { $saved | Add-Member -NotePropertyName queue -NotePropertyValue 0 -Force }
            $found = [ordered]@{ model = $saved.model; pos = @($saved.pos); queue = [int]$saved.queue }
            Send-Response $context 200 (ConvertTo-Json -InputObject $found -Depth 20 -Compress)
        }
        'lud0/wint' {
            $saved = Read-PlayerRecord $uid
            if ($null -eq $saved) { $saved = [pscustomobject]@{ alive = 1; queue = 0; ints = [pscustomobject]@{} } }
            $body = (Get-RequestBody $request) | ConvertFrom-Json
            if (-not $saved.ints) { $saved | Add-Member -NotePropertyName ints -NotePropertyValue ([pscustomobject]@{}) -Force }
            $saved.ints | Add-Member -NotePropertyName ([string]$body.col) -NotePropertyValue ([int]$body.val) -Force
            Write-PlayerRecord $uid $saved
            Send-Response $context 200 '{"ok":true}'
        }
        'lud0/rint' {
            $saved = Read-PlayerRecord $uid
            $body = (Get-RequestBody $request) | ConvertFrom-Json
            $value = 0
            if ($null -ne $saved -and $saved.ints -and $null -ne $saved.ints.([string]$body.col)) { $value = [int]$saved.ints.([string]$body.col) }
            Send-Response $context 200 ('{"int":' + $value + '}')
        }
        'lud0/queue' {
            $saved = Read-PlayerRecord $uid
            if ($null -eq $saved) { $saved = [pscustomobject]@{ alive = 1; queue = 0 } }
            $body = (Get-RequestBody $request) | ConvertFrom-Json
            $saved | Add-Member -NotePropertyName queue -NotePropertyValue ([int]$body.queue) -Force
            Write-PlayerRecord $uid $saved
            Send-Response $context 200 '{"ok":true}'
        }
        default { Send-Response $context 404 '{"error":"unknown character operation"}' }
    }
}

function Handle-World($context, [string]$route) {
    $request = $context.Request
    $items = @(Read-WorldItems)
    switch ($route) {
        { $_ -eq 'world' -or $_ -eq 'world/sys' } {
            Send-Response $context 200 '{"scurr":200,"snext":400,"stime":0,"toffset":0,"tscale":0}'
        }
        'world/add' {
            $body = (Get-RequestBody $request) | ConvertFrom-Json
            $next = 1
            if ($items.Count -gt 0) { $next = 1 + [int](($items | Measure-Object -Property id -Maximum).Maximum) }
            $record = [pscustomobject]@{ id = $next; type = [string]$body.type; pos = @($body.mt); imp = [int]$body.imp }
            if ($null -ne $body.PSObject.Properties['state']) { $record | Add-Member -NotePropertyName state -NotePropertyValue $body.state }
            if ($null -ne $body.PSObject.Properties['fuel']) { $record | Add-Member -NotePropertyName fuel -NotePropertyValue ([double]$body.fuel) }
            $items += $record
            Save-WorldItems $items
            Send-Response $context 200 ('{"id":' + $next + '}')
        }
        'world/update' {
            $body = (Get-RequestBody $request) | ConvertFrom-Json
            $item = $items | Where-Object { [int]$_.id -eq [int]$body.item } | Select-Object -First 1
            if ($null -eq $item) { Send-Response $context 404 '{"error":"item not found"}'; return }
            if ($null -ne $body.PSObject.Properties['mt']) { $item.pos = @($body.mt) }
            if ($null -ne $body.PSObject.Properties['state']) {
                if ($null -eq $item.PSObject.Properties['state']) { $item | Add-Member -NotePropertyName state -NotePropertyValue $body.state }
                else { $item.state = $body.state }
            }
            if ($null -ne $body.PSObject.Properties['fuel']) {
                if ($null -eq $item.PSObject.Properties['fuel']) { $item | Add-Member -NotePropertyName fuel -NotePropertyValue ([double]$body.fuel) }
                else { $item.fuel = [double]$body.fuel }
            }
            Save-WorldItems $items
            Send-Response $context 200 '{"ok":true}'
        }
        'world/remove' {
            $body = (Get-RequestBody $request) | ConvertFrom-Json
            $items = @($items | Where-Object { [int]$_.id -ne [int]$body.item })
            Save-WorldItems $items
            Send-Response $context 200 '{"ok":true}'
        }
        'world/get' {
            $body = (Get-RequestBody $request) | ConvertFrom-Json
            $item = $items | Where-Object { [int]$_.id -eq [int]$body.item } | Select-Object -First 1
            if ($null -eq $item) { Send-Response $context 404 '{"error":"item not found"}'; return }
            $result = [ordered]@{ type = $item.type; pos = @($item.pos) }
            if ($null -ne $item.PSObject.Properties['state']) { $result['state'] = $item.state }
            if ($null -ne $item.PSObject.Properties['fuel']) { $result['fuel'] = $item.fuel }
            Send-Response $context 200 (ConvertTo-Json -InputObject $result -Depth 40 -Compress)
        }
        'world/list' {
            $ids = @($items | Sort-Object id | ForEach-Object { [int]$_.id })
            Send-Response $context 200 (ConvertTo-Json -InputObject ([ordered]@{ count = $ids.Count; ids = $ids }) -Depth 10 -Compress)
        }
        'world/count' {
            Send-Response $context 200 ('{"count":' + $items.Count + '}')
        }
        'world/batch' {
            $body = (Get-RequestBody $request) | ConvertFrom-Json
            $beg = [int]$body.beg; $end = [int]$body.end
            $found = @($items | Where-Object { [int]$_.id -ge $beg -and [int]$_.id -le $end } | Sort-Object id)
            $result = [ordered]@{ count = $found.Count; beg = $beg; end = $end }
            foreach ($item in $found) { $result[[string]$item.id] = @([string]$item.type) + @($item.pos) }
            Send-Response $context 200 (ConvertTo-Json -InputObject $result -Depth 30 -Compress)
        }
        default { Send-Response $context 404 '{"error":"unknown world operation"}' }
    }
}

$listener = [Net.HttpListener]::new()
$listener.Prefixes.Add("http://127.0.0.1:$port/")
$listener.Start()
[IO.File]::WriteAllText($readyFile, [string]$PID, [Text.Encoding]::ASCII)
Write-StoreLog "Local DayZ persistence listening at 127.0.0.1:$port"

try {
    while ($listener.IsListening) {
        $context = $listener.GetContext()
        try {
            $route = $context.Request.Url.AbsolutePath.Trim('/')
            if ($route -eq 'health') { Send-Response $context 200 '{"status":"ready"}'; continue }
            if ($route -eq 'lud0' -or $route.StartsWith('lud0/')) { Handle-Character $context $route; continue }
            if ($route -eq 'world' -or $route.StartsWith('world/')) { Handle-World $context $route; continue }
            Send-Response $context 404 '{"error":"not found"}'
        } catch {
            Write-StoreLog ("Request error: " + $_.Exception.Message)
            try { Send-Response $context 500 '{"error":"local storage error"}' } catch { }
        }
    }
} finally {
    $listener.Stop()
    $listener.Close()
    Remove-Item -LiteralPath $readyFile -Force -ErrorAction SilentlyContinue
}
