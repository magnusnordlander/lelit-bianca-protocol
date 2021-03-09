<?php

$data = file_get_contents("log.txt");

function doStuff(string $hex1, string $hex2, ?string $extra = null): int {
    $num5h = hexdec($hex2);
    $num5 = $num5h >> 1;

    $num4h = hexdec($hex1);
    $num4 = $num4h & 0x06;
    $num4 = $num4 << 6;

    $ex = 0;

    if ($extra) {
        $e = hexdec($extra);

        if ((($e & 0x2) && ($num4h & 0x2)) || ($num4h & 0x4)) {
            $ex = 0x80;

/*            if ($e & 0x2) {
                            echo sprintf("Debug: %s %s %s %s %s\n", base_convert($num4, 10, 2), base_convert($num5, 10, 2), base_convert($ex, 10, 2), base_convert(($num4 | $num5), 10, 2), base_convert(($num4 | $num5) + $ex, 10, 2));

            echo sprintf("%s %s %s", base_convert($num4h, 10, 2), base_convert($num5h, 10, 2), base_convert($e, 10, 2));

            die();
            } */

        }
    }

    return ($num4 | $num5) + $ex;
}

echo "time,b2,b5,b8,b16,b11\n";

foreach (explode("\n", $data) as $line) {
    $parts = explode(" ", $line);

    if (count($parts) == 8) {
        // TX, from LCC
        [$frame, $dir, $time, $byte0, $byte1, $byte2, $byte3, $nul] = $parts;
    } elseif(count($parts) == 21) {
        // RX, from control board
        [$frame, $dir, $time, $byte0, $byte1, $byte2, $byte3, $byte4, $byte5, $byte6, $byte7, $byte8, $byte9, $byte10, $byte11, $byte12, $byte13, $byte14, $byte15, $byte16, $nul] = $parts;

        echo sprintf("%s,%d,%d,%d,%d,%d\n", rtrim($time, ":"), doStuff($byte1, $byte2, $byte3), doStuff($byte4, $byte5, $byte6), doStuff($byte7, $byte8, $byte9), doStuff($byte15, $byte16), doStuff($byte10, $byte11, $byte12));

        ///echo sprintf("%s %d %d %d %d\n", $time, hexdec($byte7) >> 1, hexdec($byte8) >> 1, hexdec($byte9) >> 1, doStuff($byte7, $byte8));
    }
}