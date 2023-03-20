# Meson Build System

Nix is now experimentally using [Meson](https://mesonbuild.com/) for a local build system.

## Getting Started

Use the [Hacking Guide](hacking.md) to enter a devshell. It contains all the dependencies needed for the Meson build system.

## Using Meson

Meson does out of source builds, and must be given the path for some of the dependencies. From the root of the repo once you're in the devshell, run the following:

```console
$ meson setup -D rapidcheck_dir=$RAPIDCHECK_DIR -D aws_sdk_cpp_include_dir=$AWS_SDK_INCLUDEDIR -D aws_sdk_cpp_lib_dir=$AWS_SDK_LIBRARYDIR builddir
$ cd builddir/
$ meson compile
```

To run the tests, use:

```console
$ meson test
```
