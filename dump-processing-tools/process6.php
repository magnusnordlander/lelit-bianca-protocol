<?php

$data = file_get_contents($argv[1]);

function doStuff(string $hextriplet): int {
    $dec = hexdec($hextriplet);
    $lsb = ($dec & 0x00FE00) >> 9;

    $hig = ($dec & 0x060000) >> 16;
    $mid = ($dec & 0x000002) >> 1;

    $msb = ($hig | $mid) - 1;

    return $lsb | ($msb << 7);
}

function p(bool $b) {
    return $b ? 127 : 0;
}

function q(bool $b) {
    return $b ? 63 : 0;
}


echo "time,siga,sigb,sigc,pumpon,sige,sigf,sigg,sigh,sigt\n";

foreach (explode("\n", $data) as $line) {
    $parts = explode(" ", $line);

    if (count($parts) == 8) {
        $parts = array_map(function($thing) {
            if (strlen($thing) == 1) {
                return '0'.$thing;
            }

            return $thing;
        }, $parts);

        // TX, from LCC
        [$frame, $dir, $time, $byte0, $byte1, $byte2, $byte3, $nul] = $parts;

        $sigA = (hexdec($byte0) & 0xF0) !== 0xB0;
        $sigB = (hexdec($byte0) & 0x0E) !== 0x0A;
        $sigC = (hexdec($byte1) & 0xF0) !== 0xE0;
        $pumpOn = (hexdec($byte1) & 0x0E) === 0x0E;
        $sigE = (hexdec($byte2) & 0xF0) === 0xF0;
        $sigF = (hexdec($byte2) & 0x0F) !== 0x07;
        $sigG = (hexdec($byte3) & 0x30) >> 4;
        $sigH = (hexdec($byte3) & 0x0E) >> 1;
        $sigT = (hexdec($byte3) & 0x3E) >> 1;

        echo sprintf("%s,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", str_pad(rtrim($time, ":"), 10, "0", STR_PAD_LEFT), p($sigA), q($sigB), p($sigC), p($pumpOn), p($sigE), p($sigF), $sigG, $sigH, $sigT);

//        echo sprintf("%s,%s,%d,%d,%d\n", str_pad(rtrim($time, ":"), 10, "0", STR_PAD_LEFT), str_pad(base_convert($byte0.$byte1.$byte2.$byte3, 16, 2), 32, "0", STR_PAD_LEFT), $sigA, $sigB, $pumpOn);
///        echo sprintf("%s\n", str_pad(base_convert($byte0.$byte1.$byte2.$byte3, 16, 2), 32, "0", STR_PAD_LEFT));
    } elseif(count($parts) == 21) {
        continue;
        $parts = array_map(function($thing) {
            if (strlen($thing) == 1) {
                return '0'.$thing;
            }

            return $thing;
        }, $parts);

        // RX, from control board
        [$frame, $dir, $time, $byte0, $byte1, $byte2, $byte3, $byte4, $byte5, $byte6, $byte7, $byte8, $byte9, $byte10, $byte11, $byte12, $byte13, $byte14, $byte15, $byte16, $nul] = $parts;

        $microswitchOn = (hexdec($byte1) & 0x40) === 0x00;

        echo sprintf("%s,%d,%d,%d,%d,%d,%d\n", str_pad(rtrim($time, ":"), 10, "0", STR_PAD_LEFT), doStuff($byte1.$byte2.$byte3), doStuff($byte4.$byte5.$byte6), doStuff($byte7.$byte8.$byte9), doStuff($byte10.$byte11.$byte12), doStuff($byte13.$byte14.$byte15), $microswitchOn ? 255 : 0);

        ///echo sprintf("%s %d %d %d %d\n", $time, hexdec($byte7) >> 1, hexdec($byte8) >> 1, hexdec($byte9) >> 1, doStuff($byte7, $byte8));
    }
}