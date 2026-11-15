# Earth Analog Analyzer

Earth Analog Analyzer is a C++17 program that compares selected locations on Earth with reference regions on Mars and the Moon.

The project currently uses five Earth locations:

- Atacama
- Antarctica
- Hawaii
- Iceland
- Sahara

They are compared with two selected planetary regions:

- Gale Crater on Mars
- Lacus Mortis on the Moon

The program is designed to compare three measurements:

- Surface temperature (°C)
- Terrain slope (degrees)
- Local relief (meters)

It reads the data from text files, checks which measurements are available, and compares the matching ranges. The results for Mars and the Moon are calculated separately.

This is a preliminary comparison tool. It does not prove that an Earth location is an exact match for a place on Mars or the Moon.

## Project Files

- `Earth_Analog_Analyzer_Final.cpp` — the main C++ source code.
- `earth_data.txt` — data for the selected Earth locations.
- `reference_data.txt` — reference data for Mars and the Moon.
- `README.md` — project information, method, and data sources.
- `LICENSE` — the project's license.

## How the Comparison Works

Each measurement is represented by a minimum and maximum value.

The program checks whether the Earth and planetary reference ranges are both available and valid. If a measurement is missing on either side, it is skipped. Missing values are not treated as zero.

For each available pair of ranges, the program calculates the overlap using IoU (Intersection over Union).

The formula is:

```text
IoU = (Overlap / Union) × 100
```

Where:

- **Overlap** is the shared part of the two ranges.
- **Union** is the total range covered by both intervals.

A score of 100% means the two ranges are identical, while 0% means they do not overlap.

The program calculates the average IoU across the measurements that can be compared. Locations are then ordered by their average score. If scores are equal, the program gives priority to the location with more comparable measurements.

The program also shows how many criteria were used in each result.

## Earth Data Sources

### Surface Temperature

Surface temperature data for the selected Earth locations was collected from NASA's MODIS Terra MOD11A2.061 dataset for 2025.

The dataset provides Land Surface Temperature measurements every 8 days, at approximately 1 km resolution. Daytime and nighttime measurements were converted to Celsius and combined to determine the minimum and maximum temperatures for each location across the 2025 observations.

The original values were converted using:

- Kelvin = Raw value × 0.02
- Celsius = Kelvin − 273.15

Source: [NASA MODIS MOD11A2.061](https://lpdaac.usgs.gov/products/mod11a2v061/)

**Note:** These temperature ranges are preliminary. Quality-control filtering was not fully applied to all observations.

### Terrain Slope

Slope data was collected from NASADEM Slope and Curvation Global 1 arc second V001.

The dataset provides slope measurements at approximately 1 arc-second resolution. The stored values were converted to degrees using the scale factor of 0.01.

For each selected location, slope values were extracted from an approximately 20 × 20 km rectangular window around the location. The minimum and maximum values represent the slope range within this window, not across the entire geographic region.

Slope data was available for Atacama, Hawaii, and Sahara. No values were available for Antarctica and Iceland in the selected dataset coverage.

Source: [NASADEM Slope and Curvation Global 1 arc second V001](https://lpdaac.usgs.gov/products/nasadem_scv001/)

### Local Relief

Local relief values are not included in the current dataset. NASADEM elevation data was considered, but a consistent local-relief measurement was not calculated for all selected locations.

Absolute elevation was not used as a replacement, since it describes a different terrain measurement.

For this reason, all relief values are marked as `?;?` in the data file and are skipped by the program.

Reference dataset considered: [NASADEM Merged DEM Global 1 arc second V001](https://lpdaac.usgs.gov/products/nasadem_hgtv001/).

## Planetary Reference Data

### Mars — Gale Crater

The selected Mars region is Gale Crater, near the Curiosity rover landing area.

The temperature range is based on ground-temperature measurements reported by Martínez et al. (2016). The reported values are approximately 173 K to 289 K, converted to:

- Minimum: -100.15 °C
- Maximum: 15.85 °C

The two temperature extremes were measured at different times and rover positions along the Curiosity traverse. They represent an observed range for the Gale region, not simultaneous measurements across the landing area.

Slope and local-relief values are marked as missing because the available measurements did not provide matching minimum and maximum ranges for the selected area.

Source: Martínez et al. (2016), [Analysis of the environmental conditions at Gale Crater from MSL/REMS measurements](https://revistas.ucm.es/index.php/FITE/es/article/download/53902/49342).

### Moon — Lacus Mortis

The selected lunar region is Lacus Mortis, around the landing ellipse considered for Astrobotic Mission One.

The temperature range is based on a Diviner LRO study by Williams et al. (2022). The reported values are approximately 88 K to 359 K, converted to:

- Minimum: -185.15 °C
- Maximum: 85.85 °C

Slope and local-relief values are currently marked as missing because the corresponding local data has not yet been extracted and checked for the selected area.

Source: Williams et al. (2022), [Temperatures of the Lacus Mortis Region of the Moon](https://doi.org/10.1029/2021EA001966).

## Current Data Status

The project is designed to compare all three measurements, but not all the required data is currently available.

At this stage:

- Earth temperature data is available for all five locations.
- Earth slope data is available for Atacama, Hawaii, and Sahara.
- Earth local-relief data is missing for all five locations.
- The planetary reference files currently contain temperature ranges only.

This means that **surface temperature is currently the only criterion that can be compared on both sides** for the selected Mars and Moon regions.

Slope and local relief are skipped by the program until suitable reference data is added.

## Limitations

- The current results are based on temperature ranges only, so the comparison is limited.
- Earth MODIS surface temperature, Mars rover ground temperature, and lunar Diviner surface temperature come from different instruments and observation methods.
- Slope measurements depend on the resolution of the dataset and the area used for extraction.
- Absolute elevation and local relief are different measurements and should not be treated as interchangeable.
- Minimum and maximum values can be affected by unusual observations.
- Missing measurements are excluded from the calculation, not treated as zero.
- The IoU score measures range overlap only. It is not a probability and does not prove that two locations are scientifically equivalent.

The results should therefore be considered an initial screening, not a final scientific assessment of Earth analog locations.

## How to Run

The program requires a C++17 compiler.

Compile the source code using:

```bash
g++ -std=c++17 -O2 Earth_Analog_Analyzer_Final.cpp -o analyzer
```

Run the program from the folder containing the two data files:

```bash
./analyzer
```

You can also provide different input file names:

```bash
./analyzer earth_data.txt reference_data.txt
```

To run the built-in tests for the comparison calculations:

```bash
./analyzer --test
```

On Windows, run the compiled program as:

```bash
analyzer.exe
```

## Input File Format

Both data files use semicolon-separated values.

The Earth data file contains:

```text
name;source;temp_min;temp_max;slope_min;slope_max;relief_min;relief_max
```

The planetary reference file contains:

```text
target;region;source;temp_min;temp_max;slope_min;slope_max;relief_min;relief_max
```

Missing measurements are written as `?;?`.

The program skips missing pairs and only uses valid ranges for comparison.

## Tests

The program includes 17 built-in tests to check the main calculations and input validation.

The tests cover:
- Range overlap calculations.
- Invalid and reversed ranges.
- Missing measurements.
- Criteria counting and averaging.
- Ranking based on IoU scores.

All 17 tests passed successfully.

**Test result: 17 passed, 0 failed.** 
analyzer.exe
analyzer.exe --test

## License

The original code and documentation of this project are licensed under the Apache License, Version 2.0.

Copyright 2026 Deema Shaker.

You are free to use, modify, and distribute the original project files under the terms of this license.

See the [LICENSE](LICENSE) file for the full license text.

Third-party datasets and scientific references are not covered by this license. They remain subject to their own terms and are credited in the relevant sections above.