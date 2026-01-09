param(
    [ValidateSet("uart", "jtag")]
    [string]$Mode = "uart"
)

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$sdk = Join-Path $root "sdkconfig"
if (-not (Test-Path $sdk)) {
    Write-Error "sdkconfig not found: $sdk"
    exit 1
}

$text = Get-Content -Raw $sdk
$nl = if ($text.Contains("`r`n")) { "`r`n" } else { "`n" }

function Set-ConfigValue([string]$src, [string]$key, [string]$value) {
    $pattern1 = "(?m)^\s*$key=.*$"
    $pattern2 = "(?m)^\s*#\s*$key\s+is not set\s*$"
    if ($src -match $pattern1 -or $src -match $pattern2) {
        $src = [regex]::Replace($src, $pattern1, "$key=$value")
        $src = [regex]::Replace($src, $pattern2, "$key=$value")
    } else {
        $src += "$nl$key=$value"
    }
    return $src
}

function Set-ConfigNotSet([string]$src, [string]$key) {
    $pattern1 = "(?m)^\s*$key=.*$"
    $pattern2 = "(?m)^\s*#\s*$key\s+is not set\s*$"
    $line = "# $key is not set"
    if ($src -match $pattern1 -or $src -match $pattern2) {
        $src = [regex]::Replace($src, $pattern1, $line)
        $src = [regex]::Replace($src, $pattern2, $line)
    } else {
        $src += "$nl$line"
    }
    return $src
}

if ($Mode -eq "uart") {
    $text = Set-ConfigValue $text "CONFIG_ESP_CONSOLE_UART_DEFAULT" "y"
    $text = Set-ConfigNotSet $text "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG"
    $text = Set-ConfigValue $text "CONFIG_ESP_CONSOLE_UART" "y"
    $text = Set-ConfigValue $text "CONFIG_ESP_CONSOLE_UART_NUM" "0"
    $text = Set-ConfigValue $text "CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM" "0"
    $text = Set-ConfigValue $text "CONFIG_CONSOLE_UART_DEFAULT" "y"
    $text = Set-ConfigValue $text "CONFIG_CONSOLE_UART" "y"
    $text = Set-ConfigValue $text "CONFIG_CONSOLE_UART_NUM" "0"
} else {
    $text = Set-ConfigNotSet $text "CONFIG_ESP_CONSOLE_UART_DEFAULT"
    $text = Set-ConfigValue $text "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG" "y"
    $text = Set-ConfigNotSet $text "CONFIG_ESP_CONSOLE_UART"
    $text = Set-ConfigValue $text "CONFIG_ESP_CONSOLE_UART_NUM" "-1"
    $text = Set-ConfigValue $text "CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM" "4"
    $text = Set-ConfigNotSet $text "CONFIG_CONSOLE_UART_DEFAULT"
    $text = Set-ConfigNotSet $text "CONFIG_CONSOLE_UART"
    $text = Set-ConfigValue $text "CONFIG_CONSOLE_UART_NUM" "-1"
}

Set-Content -Path $sdk -Value $text -NoNewline
Write-Host "sdkconfig console switched to $Mode"
