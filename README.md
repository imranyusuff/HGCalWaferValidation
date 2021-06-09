# HGCalWaferValidation

A CMSSW module that validates HGCal wafer data inside DD against specifications given in a flat text file.

## Quick build

Set up the CMSSW workarea (`CMSSW_12_0_0_pre1` was used for development; any recent version should work too):

```
cd $HOME/your/CMSSW/work/area
cmsrel CMSSW_12_0_0_pre1
cd CMSSW_12_0_0_pre1/src
cmsenv
```

Now clone this repository and build it.

```
git clone https://github.com/imranyusuff/HGCalWaferValidation.git UserCode/HGCElectronicsValidation
scram b   # Add '-j 4' to use 4 cores
```

To run this module, you need to get the reference geometry text file and put it somewhere accessible by you.
Then, under the `python` folder, edit the config scripts to specify this file, under `GeometryFileName` field
of module instantiation. Once such file could be `geomnew_corrected_360.txt`.

## Running the validation module

Make sure you have edited the scripts in `python` directory to point to the geometry text file.

Use `cmsRun` to run the validation module. To check D49 geometry, enter:

```
cmsRun UserCode/HGCalWaferValidation/python/geoD49_test1_HGCalWaferValidation_cfg.py
```

To check D83 geometry, enter:

```
cmsRun UserCode/HGCalWaferValidation/python/geoD83_test1_HGCalWaferValidation_cfg.py
```

To check for other geometry:

```
cmsRun UserCode/HGCalWaferValidation/python/general_test1_HGCalWaferValidation_cfg.py tag=2026 version=DXX
```

where `DXX` is the geometry version you want to check.

## Implementation notes

For now, please refer to the comments on the module's source file header.
