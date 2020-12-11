# ML4NP Simulation & Macros
In this repo, we collect the code used for simulation and post-processing of simulated data.

It is composed by the following tools:
- Detection Slicer: given a G4 simulation file (flat format), it simulates the detections using the Optical and Spatial Maps (link).
- Snapshot Creator: given a simulation with the DetectionSlicer, it export a `csv` file of aggregated acquisition over a given time window.
- Macros: these are various root macros used for analysis or processing simulation files.

## Detection Slicer

## Snapshot Creator

## Main Macros
### Extraction Muon events in coincidence with Germanium
One of the analysis performed consists of considering the performance of the trained model considering the coincidence with Germanium.

#### How to run
To perform the selection in coincidence with Germanium depositions, you have to use the macro `extractEventsWtGeCoincidence.cpp`.

This macro scans a bunch of simulation files (*G4 file in flat format*):
- **Input**: files `prefix*root` contained in a `input directory`
- **Output**: write the exports in the `output directory`

In particular, the macro will process each event individually, writing 1 file for each event and then merging all of them in a unique file.

This macro has been created to process a small amount of events (i.e., 36 in our study) and then is so disk-consuming.
The macro can be improved by writing directly in a single output file (*TODO*).

##### Default configuration
From the current directory, run the following command:

```
$>root
root [0] .L macros/extractEventsWtGeCoincidence.cpp
root [1] extractEventsWtGeCoincidence()
```

or directly:

```
$>root macros/extractEventsWtGeCoincidence.cpp
```

##### IO Management 
The input-output can be managed with the following parameters:
|        Parameter          | Description |
| ------------------------- | ----------- |
| `dirIn`                   | input directory |
| `filePrefix`              | prefix of the input simulation files *(default="output")* |
| `dirOut`                  | output directory |
| `outPrefix`               | prefix of the output files produced *(default="ExportCriticalEvent")* |

**Note:** The input and output directories must exist before running the macro. (TODO)

For example, suppose you want to process the files located in another directory `/home/data/another_simulation_dir/` and all these files start with the prefix `output`.
Then, you can run the following commands:

```
$>root
root [0] .L macros/extractEventsWtGeCoincidence.cpp
root [1] extractEventsWtGeCoincidence("/home/data/another_simulation_dir/")
```

Instead, if the files in this directory start with a different prefix `simulation_file`.
Then, you can run the following commands:

```
$>root
root [0] .L macros/extractEventsWtGeCoincidence.cpp
root [1] extractEventsWtGeCoincidence("/home/data/another_simulation_dir/", "simulation_file")
```

##### Filter Customization
The selection can be customized with the following parameters:
|        Parameter           | Description |
| -------------------------- | ----------- |
| `minGeDep`                 | minimum amount of energy deposited (*Kev*) in the Germanium detector (*default=1839 KeV*) |
| `maxGeDep`                 | maximum amount of energy deposited (*Kev*) in the Germanium detector (*default=2239 KeV*) |
| `minLArDep`                | minimum amount of energy deposited (*Kev*) in the Liquid Argon (*default=0 KeV*) |
| `maxLArDep`                | maximum amount of energy deposited (*Kev*) in the Liquid Argon (*default=20 000 KeV*) |
| `minGeMultiplicity`        | minimum number of Germanium detectors in which the deposition occurs (*default=1*)
| `maxGeMultiplicity`        | maximum number of Germanium detectors in which the deposition occurs (*default=1*)

For example, suppose you want to filter all the events with at least `15 KeV` deposited in the Germanium detectors.
Then, you can run the following commands:

```
$>root
root [0] .L macros/extractEventsWtGeCoincidence.cpp
root [1]
root [2] double minGeDep = 15;              // minimum 15 KeV in Ge
root [3] double maxGeDep = 1000000000;      // arbitrary high value
root [4]
root [5] extractEventsWtGeCoincidence("data/", "output", minGeDep, maxGeDep)
```
**Note:** since we don't want any upperbound on the Germanium detections, we use an arbitrary high-value (e.g., 1000000000).

Similarly, if you want to filter such events with also at most `10 MeV` in Liquid Argon, then you can run the following commands:

```
$>root
root [0] .L macros/extractEventsWtGeCoincidence.cpp
root [1]
root [2] double minGeDep = 15;              // minimum 15 KeV in Ge
root [3] double maxGeDep = 1000000000;      // arbitrary high value
root [4] double minLArDep = 0;              // no lowerbound on LAr acquisitions (take also zero detections)
root [5] double maxLArDep = 10000;          // maximum 10 MeV in LAr
root [6]
root [7] extractEventsWtGeCoincidence("data/", "output", minGeDep, maxGeDep, minLArDep, maxLArDep)
```

Finally, if you want to have only events with all the Germanium deposition in maximum 3 detectors (i.e., crystals), then you can run the following commands:

```
$>root
root [0] .L macros/extractEventsWtGeCoincidence.cpp
root [1] double minGeDep = 15;              // minimum 15 KeV in Ge
root [2] double maxGeDep = 1000000000;      // arbitrary high value
root [3] double minLArDep = 0;              // no lowerbound on LAr acquisitions (take also zero detections)
root [4] double maxLArDep = 10000;          // maximum 10 MeV in LAr
root [5] int minGeMult = 1;                 // at least 1 Ge crystal
root [6] int maxGeMult = 3;                 // maximum 3 Ge crystals
root [7]
root [8] extractEventsWtGeCoincidence("data/", "output", minGeDep, maxGeDep, minLArDep, maxLArDep, minGeMult, maxGeMult)
```