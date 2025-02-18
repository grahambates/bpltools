# Bitplane tools

## bplconv

Converts 8 bit indexed PNGs to bitplane data

```
Usage: bplconv [options] <image.png> <output_file>
Options:
  -i, --interleaved          Enable interleaved mode
  -r, --raw-palette=FILE     Export raw palette
  -c, --copper-palette=FILE  Export palette as copper list
  -v, --verbose              Enable verbose output
  -h, --help                 Display this help message
```

## bplopt

Reorders palette of an indexed PNG for optimal LZ compression of converted bitplane data.

Inspired by [BPLOptimize by TEK](https://www.pouet.net/prod.php?which=71288). Thanks Bifat <3

```
Usage: bplopt [options] <input.png> <output.png>
Options:\n");
  -i, --interleaved          Enable interleaved mode
  -s, --simulated-annealing  Use simulated annealing
  -l, --lock=INDEXES         Lock palette indexes (comma separated)
  -v, --verbose              Enable verbose output
  -h, --help                 Display this help message
```

## Compiling

Dependencies: `libpng`, `zlib`, `pkg-config`

On Mac:
```
brew install libpng zlib pkg-config
git clone git@github.com:grahambates/bpltools.git
cd bpltools
make
```
