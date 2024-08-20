<a href="https://scan.coverity.com/projects/rodan-eecu-sat">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/30615/badge.svg"/>
</a>

## eecu-sat (Engine ECU signal acquisition tool)

set of multiple modules that allow all analog signals generated and received by a vehicle's Engine Electronic Control Unit to be captured and analyzed. it can either be used as a testing jig for EECUs or as a low level diagnostic tool in which a known good set of signals can be used as reference.

![logo](./doc/img/esat_analog_modules.jpg)

a number of filters and buffers are used to protect the ADCs from transient spikes generated by all inductive loads

```
 source:       https://github.com/rodan/eecu-sat
 author:       Petre Rodan <2b4eda@subdimension.ro>
 license:      BSD
```

more images of the prototype in action are available [here](https://photos.app.goo.gl/Gay5FS8gsCTZkYcH9)

### Project components

project directory structure

 * ./hardware/buffy - kicad based schematics and pcbs for the analog buffer stage
 * ./hardware/pb\_jst-zro*  - kicad based schematics and pcbs for the proxy boards that intercept all EECU signals for a KTM 1090 EECU that uses JST ZRO 36 and 48 pin connectors
 * ./ltspice - spice simulation of the analog buffer schematic
 * ./software/eecu-sat - program that processes the raw analog data exported by [Saleae's Logic](https://www.saleae.com/pages/downloads) and converts them into [sigrok](https://sigrok.org/wiki/File_format:Sigrok/v2) files
 * .doc/ktm\_1090\_wiring.ods - ktm 1090 ECU pinout based on wire harness specs present in the service manual

### Build requirements

notable dependencies would be a Linux based OS, gcc, make and libzip. compilation is as simple as

```
cd ./software/eecu-sat
make
```

### TODO list (In Progress)

- [ ] 3-point calibration of all analog channels
  - [ ] use ini file to input calibration paramenters
  - [ ] analyze analog calibration signals, detect the calibration points
  - [ ] apply acceptance criteria to input calibration signal
  - [ ] export slopes and offsets into calibration ini file
- [ ] synchronize multiple sets of 16 channel acquisitions into one 48 channel srzip file
- [ ] analyze all 48 channels and automatically detect faults based on a known good reference capture

### Done

- [x] cover project with coverity static scan
- [x] convert Logic exported raw analog signals into srzip files that can be loaded in pulseview
- [x] relabeling capability of channels when seen in pulseview


![tool in use](./doc/img/esat_in_use.jpg)

