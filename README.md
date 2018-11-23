# tinyhost
Individual-based model of resistant disease transmission

## Compilation
`tinyhost` can be compiled using GNU Make and GCC. To compile, run
```
make
```
from the Tinyhost directory.

## Usage
```
./tinyhost [cfgfile [setnumber]] {-parameter value}
```

When `tinyhost` is run, it shows a list of parameters and their current values. The meaning of parameters is documented in the file config_def.h.

If a config file and optional parameter-set number is specified in the first one or two arguments, e.g.

```
./tinyhost ./Runs/1-Steps/neu.config.cfg 1
```

then parameters are loaded from the specified config file. There are many examples in the "Runs" directory. The parameter-set number following the path to the config file indicates which parameter set to run from the config file, with 1 being the first set in the file. It can either be a single integer, or a range like 1-3 (inclusive). Do not put spaces between the start, hyphen, or end of the range. If the config number is omitted, all parameter sets in the config file are run individually.

Additionally or alternatively, parameters can be overridden from the command line like so:

```
./tinyhost -k 0.5 -t_step 0.1
```

the name of the parameter preceded by a hyphen is followed by a space, then the value of the parameter. To include a space in the value of a parameter, the entire value must be surrounded by quotation marks, e.g.

```
./tinyhost -fileout "./My output/run 7.txt"
```

Note that the `fileout` parameter is interpreted in a special way: if it is the same for multiple consecutive parameter sets specified through a config file, then the results of these multiple runs will be appended to the same output file. If `fileout` changes between parameter sets (or at the beginning of a run of `tinyhost`), then existing contents are overwritten.

Some parameters (e.g. `beta`) are vector parameters, in that they take a sequence of values. Separate each value using commas, like so:

```
./tinyhost -beta 4,3.8
```

with just commas, no spaces.
