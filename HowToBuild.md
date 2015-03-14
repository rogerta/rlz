# Build environment #

RLZ uses the same build environment as Chromium.  Follow the instructions in the section [Build Environment](http://www.chromium.org/developers/how-tos/build-instructions-windows#TOC-Build-environment), up to and including the step "Get the Chromium depot\_tools".

Do not checkout the Chromium code.

# Checking out the RLZ code #

Open a Command Prompt window and execute the following commands:

```
C:
mkdir \src\RLZ
cd \src\RLZ
gclient config https://src.chromium.org/chrome/trunk/src/rlz --name src/rlz
gclient sync
```

The last command may take a few minutes to complete, depending on the speed of your Internet connection.

At any time re-run the `gclient sync` command to checkout the latest revision of the RLZ code.

## Using Visual Studio 2010 ##

When using Visual Studio 2010, define the environment variable `GYP_MSVS_VERSION` before running gclient sync:

```
set GYP_MSVS_VERSION=2010
gclient sync
```

# Building the RLZ code #

`gclient sync` creates a Visual Studio solution file called `C:\src\RLZ\src\rlz\rlz.sln` for the entire RLZ project.  To build, either open this solution in the Visual Studio IDE or run the following command in a Command Prompt window:

`devenv /build Debug rlz.sln /project rlz_unittests.vcproj`

Run `C:\src\RLZ\src\build\Debug\rlz_unittests.exe` and make sure all tests pass.

To build and test a release version of RLZ, use the following commands:

```
devenv /build Release rlz.sln /project rlz_unittests.vcproj
C:\src\RLZ\src\build\Release\rlz_unittests.exe
```