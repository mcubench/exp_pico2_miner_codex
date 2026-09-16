#!/usr/bin/env python3
"""Enumerate exact RP2350 system-clock PLL points used by Pico SDK 2.3.1."""

import argparse


XOSC_KHZ = 12_000
VCO_MIN_KHZ = 750_000
VCO_MAX_KHZ = 1_600_000


def catalog(min_khz: int, max_khz: int) -> dict[int, tuple[int, int, int]]:
    points: dict[int, tuple[int, int, int]] = {}
    # Match check_sys_clock_khz ordering so the tuple is the one the SDK uses.
    for fbdiv in range(320, 15, -1):
        vco_khz = fbdiv * XOSC_KHZ
        if not VCO_MIN_KHZ <= vco_khz <= VCO_MAX_KHZ:
            continue
        for postdiv1 in range(7, 0, -1):
            for postdiv2 in range(postdiv1, 0, -1):
                divisor = postdiv1 * postdiv2
                if vco_khz % divisor:
                    continue
                output_khz = vco_khz // divisor
                if min_khz <= output_khz <= max_khz:
                    points.setdefault(output_khz, (vco_khz, postdiv1, postdiv2))
    return dict(sorted(points.items()))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--min-khz", type=int, default=150_000)
    parser.add_argument("--max-khz", type=int, default=590_000)
    parser.add_argument("--nearest-khz", type=int)
    args = parser.parse_args()
    points = catalog(args.min_khz, args.max_khz)
    if args.nearest_khz is not None:
        if not points:
            parser.error("selected range contains no realizable PLL points")
        frequency = min(points, key=lambda item: (abs(item - args.nearest_khz), item))
        vco, postdiv1, postdiv2 = points[frequency]
        print("requested_khz\trealized_khz\tvco_khz\tpostdiv1\tpostdiv2")
        print(f"{args.nearest_khz}\t{frequency}\t{vco}\t{postdiv1}\t{postdiv2}")
        return 0
    print("realized_khz\tvco_khz\tpostdiv1\tpostdiv2")
    for frequency, (vco, postdiv1, postdiv2) in points.items():
        print(f"{frequency}\t{vco}\t{postdiv1}\t{postdiv2}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
