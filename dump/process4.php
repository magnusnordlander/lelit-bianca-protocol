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

echo "time,unkn1,unkn2,brewboiler,serviceboiler,waterlevel,microswitch\n";

foreach (explode("\n", $data) as $line) {
    $parts = explode(" ", $line);

    if (count($parts) == 8) {
        // TX, from LCC
        [$frame, $dir, $time, $byte0, $byte1, $byte2, $byte3, $nul] = $parts;
    } elseif(count($parts) == 21) {
        $parts = array_map(function($thing) {
            if (strlen($thing) == 1) {
                return '0'.$thing;
            }

            return $thing;
        }, $parts);

        // RX, from control board
        [$frame, $dir, $time, $byte0, $byte1, $byte2, $byte3, $byte4, $byte5, $byte6, $byte7, $byte8, $byte9, $byte10, $byte11, $byte12, $byte13, $byte14, $byte15, $byte16, $nul] = $parts;

        $microswitchOn = (hexdec($byte1) & 0x40) === 0x00;

        echo sprintf("%s,%d,%d,%d,%d,%d,%d\n", rtrim($time, ":"), doStuff($byte1.$byte2.$byte3), doStuff($byte4.$byte5.$byte6), doStuff($byte7.$byte8.$byte9), doStuff($byte10.$byte11.$byte12), doStuff($byte13.$byte14.$byte15), $microswitchOn ? 255 : 0);

        ///echo sprintf("%s %d %d %d %d\n", $time, hexdec($byte7) >> 1, hexdec($byte8) >> 1, hexdec($byte9) >> 1, doStuff($byte7, $byte8));
    }
}