# systemc-tlm-rgb2gray-platform

This checkout contains the native SystemC/TLM RGB-to-gray platform plus the
helper scripts used to convert images to and from the raw format expected by
the simulator.

## What is included

- `scripts/image_to_raw.py` converts a JPEG/PNG into the raw RGB input format
- `scripts/raw_to_image.py` converts raw output back into a viewable image
- `src/main.cpp` is the native SystemC entry point

## Install Python package Pillow
python3 -m pip install pillow

## Install SystemC

If you do not already have SystemC, the most reliable path is to build it
from source and point this project at that install directory.

On Ubuntu or Debian, install the basic build tools first:

```bash
sudo apt update
sudo apt install -y build-essential cmake make wget tar
```

Then download the current SystemC release from the official SystemC/Accellera
site, unpack it, and build it with a prefix of your choice. A typical source
build looks like this:

```bash
cd /tmp
mkdir systemc-source
cd systemc-source
wget https://github.com/accellera-official/systemc/archive/refs/tags/3.0.2.tar.gz
tar -xf 3.0.2.tar.gz 
cd systemc-3.0.2/
../configure --prefix=/opt/systemc
make -j"$(nproc)"
sudo make install
```

## Native SystemC build

If you have SystemC installed, build the C++ version with:

```bash
make native-build SYSTEMC_HOME=/path/to/systemc
```

Then run it with:

```bash
make native-run SYSTEMC_HOME=/path/to/systemc
```

The native binary expects the same default input and output files:

- input: `pictures/raw/grumpy-online.raw`
- output: `build/native-output.raw`

If your SystemC installation uses a different include or library directory,
override `SYSTEMC_INCLUDE` and `SYSTEMC_LIBDIR` as needed.

## Run Native SystemC

Use this flow when you want to run the actual SystemC/TLM version.

1. Build the native binary:

```bash
make SYSTEMC_HOME=/opt/systemc
```

2. Convert a JPEG or PNG into the raw RGB input expected by the simulator:

```bash
python3 scripts/image_to_raw.py pictures/jpg/grumpy-cat.jpg build/proc_input.raw --resize
```

3. Run the SystemC binary:

```bash
./rgb2gray build/proc_input.raw build/output.raw
```

4. Convert the grayscale raw output back into a viewable image:

```bash
python3 scripts/raw_to_image.py build/output.raw build/output.png
```

Notes:

- The native entry point reads a raw RGB input file, not a JPEG directly.
- The binary is written to `./rgb2gray` by the current CMake settings.
- If you want to use the sample image already in the repo, convert
  `pictures/jpg/grumpy-cat.jpg` or another image into `build/input.raw` first.


## Optional image conversion

If you want to convert one of the sample JPEGs into raw input first:

```bash
python3 scripts/image_to_raw.py pictures/jpg/grumpy-cat.jpg build/grumpy-cat.raw --resize
```

To inspect a raw file:

```bash
python3 scripts/raw_to_image.py build/grumpy-online-gray.raw build/grumpy-online-gray.png --mode gray
```

## Make targets

```bash
make run
make image
```

## Notes

- The grayscale conversion uses an integer BT.601 luminance approximation.
- A full native SystemC build would still require the missing `systemc`
  headers/libraries and a C++ compiler, which are not available in this
  environment.
