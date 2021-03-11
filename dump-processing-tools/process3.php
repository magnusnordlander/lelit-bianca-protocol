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

echo "time,b8,h,b,eb\n";

$last = "";

foreach (explode("\n", $data) as $line) {
    $parts = explode(" ", $line);

    if (count($parts) == 8) {
        // TX, from LCC
        [$frame, $dir, $time, $byte0, $byte1, $byte2, $byte3, $nul] = $parts;
    } elseif(count($parts) == 21) {
        // RX, from control board

        $parts = array_map(function($thing) {
            if (strlen($thing) == 1) {
                return '0'.$thing;
            }

            return $thing;
        }, $parts);

        [$frame, $dir, $time, $byte0, $byte1, $byte2, $byte3, $byte4, $byte5, $byte6, $byte7, $byte8, $byte9, $byte10, $byte11, $byte12, $byte13, $byte14, $byte15, $byte16, $nul] = $parts;

        $thresh = 62962244;

        $hex = $byte7.$byte8.$byte9;
        $val = doStuff($hex);

        if ($hex !== $last) {
            echo sprintf("%d,%s,%s,%s\n", $val, $hex, str_pad(base_convert($hex, 16, 2), 24, "0", STR_PAD_LEFT), str_pad(base_convert($val, 10, 2), 12, "0", STR_PAD_LEFT));            
        }
        $last = $hex;

        ///echo sprintf("%s %d %d %d %d\n", $time, hexdec($byte7) >> 1, hexdec($byte8) >> 1, hexdec($byte9) >> 1, doStuff($byte7, $byte8));
    }
}