# sysfilter

Docker container for the `sysfilter` project (tested on commit hash 1469319ba6ea7cab87638c1f879541e78d72d470).
Source: https://gitlab.com/Egalito/sysfilter

Build using from the current folder (folder of the `Dockerfile`):
```sh
sudo docker build -t sysfilter_project .
```

Run using:
```sh
sudo docker run --rm -it sysfilter_project
```

Run the tool using (from the current working directory - `/tool/sysfilter/extraction`):
```sh
app/build_x86_64/sysfilter_extract <path to program to analyze>
```

## Modifying the image
1. It is possible to build faster by specifying a higher number after `make -j8`, for example `make -j16`, to allow for 16 concurrent jobs. But this will use more CPU cores and more RAM.
2. You can install whichever program you need in the top `apt-get install` statement, or directly in the container once running
3. A couple of dependencies can be removed if you wish such as `gdb` or `vim`, they were useful for debugging the container until it was working.
