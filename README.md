miku
==

```bash
$ git clone https://github.com/adrien1018/miku.git
$ cd miku
$ git submodule update --init
$ cd isolate && git am ../patches/*.patch
```

#### How to start the judge client
```bash
$ make
$ sudo ./start_script
```
Some configuration can be made inside startscript:
- `-v/--verbose` for extra verbosity
- `-p/--parallel [NUMBER]` to evaulate at most `[NUMBER]` testdata at the same time.
- `-b [NUMBER]` to set sandbox indexing offset. Need to be set to an appropriate number if running multiple judges on one computer.
- `-a/--aggressive-update` to aggressivly update verdict and result.
