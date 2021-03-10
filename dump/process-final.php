<?php

$data = file_get_contents($argv[2]);

$mode = strtolower($argv[1]) == 'coli' ? 'COLI' : 'CILO';

function transformHextripet(string $hextriplet): int {
    $dec = hexdec($hextriplet);
    $lsb = ($dec & 0x00FE00) >> 9;

    $hig = ($dec & 0x060000) >> 16;
    $mid = ($dec & 0x000002) >> 1;

    $msb = ($hig | $mid) - 1;

    return $lsb | ($msb << 7);
}

function transformBrewTemp(int $brewnumber): float {
    // 754 = 24 deg C
    // 173 = 93 deg C

    $k = ((93 - 24) / (754 - 173))*-1;
    $m = (($k * 754) - 24)*-1;

    return $k*$brewnumber+$m;
}

function transformServiceTemp(int $servicenumber): float {
    // 735 = 24 deg C
    // 63 = 125 deg C

    $k = ((125 - 24) / (735 - 63))*-1;
    $m = (($k * 735) - 24)*-1;

    return $k*$servicenumber+$m;
}

function p(bool $b) {
    return $b ? 127 : 0;
}

if ($mode == 'CILO') {
    echo "time,serviceboilerhe,brewboilerhe,pumpon\n";
} else {
    echo "time,brewboiler,serviceboiler,waterlevel,microswitch\n";
}

foreach (explode("\n", $data) as $line) {
    $parts = explode(" ", $line);

    $parts = array_map(function($thing) {
        if (strlen($thing) == 1) {
            return '0'.$thing;
        }

        return $thing;
    }, $parts);

    if (count($parts) == 8) {  
        if ($mode !== 'CILO') {
            continue;
        }

        // TX, from LCC
        [$frame, $dir, $time, $byte0, $byte1, $byte2, $byte3, $nul] = $parts;

        $sigA = (hexdec($byte0) & 0xF0) !== 0xB0;
        $brewboilerhe = (hexdec($byte0) & 0x0E) === 0x0A;
        $serviceboilerhe = (hexdec($byte1) & 0xF0) === 0xE0;
        $pumpOn = (hexdec($byte1) & 0x0E) === 0x0E;
        $sigE = (hexdec($byte2) & 0xF0) === 0xF0;
        $sigF = (hexdec($byte2) & 0x0F) !== 0x07;
        $sigG = (hexdec($byte3) & 0x30) >> 4;
        $sigH = (hexdec($byte3) & 0x0E) >> 1;
        $sigT = (hexdec($byte3) & 0x3E) >> 1;

        echo sprintf("%s,%d,%d,%d\n", str_pad(rtrim($time, ":"), 10, "0", STR_PAD_LEFT), p($brewboilerhe), p($serviceboilerhe), p($pumpOn));
    } elseif(count($parts) == 21) {
        if ($mode !== 'COLI') {
            continue;
        }

        // RX, from control board
        [$frame, $dir, $time, $byte0, $byte1, $byte2, $byte3, $byte4, $byte5, $byte6, $byte7, $byte8, $byte9, $byte10, $byte11, $byte12, $byte13, $byte14, $byte15, $byte16, $nul] = $parts;

        $microswitchOn = (hexdec($byte1) & 0x40) === 0x00;

        $brewboiler = transformHextripet($byte7.$byte8.$byte9);
        $serviceboiler = transformHextripet($byte10.$byte11.$byte12);
        $waterlevel = transformHextripet($byte13.$byte14.$byte15);

        echo sprintf("%s,%.01f,%.01f,%d,%d\n", str_pad(rtrim($time, ":"), 10, "0", STR_PAD_LEFT), transformBrewTemp($brewboiler), transformServiceTemp($serviceboiler), $waterlevel, $microswitchOn ? 255 : 0);
    }
}