# Lucy

## How to Use

1. If you use Linux, install SFML's dependencies using your system package manager. On Ubuntu and other Debian-based distributions you can use the following commands:
   ```
   sudo apt update
   sudo apt install \
       libxrandr-dev \
       libxcursor-dev \
       libxi-dev \
       libudev-dev \
       libfreetype-dev \
       libflac-dev \
       libvorbis-dev \
       libgl1-mesa-dev \
       libegl1-mesa-dev \
       libfreetype-dev
   ```
2. MacOS instructions:

   Be sure to run these commands in the root directory of this project.

   ```
   cd vcpkg
   ./bootstrap-vcpkg.sh
   ./vcpkg install opencv4:arm64-osx sfml:arm64-osx tgui:arm64-osx

   cmake -B build \
         -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake \
         -DVCPKG_TARGET_TRIPLET=arm64-osx \
         -DOpenCV_ROOT="$(pwd)/vcpkg/installed/arm64-osx/share/opencv4" \
         -DCMAKE_OSX_ARCHITECTURES=arm64

   cmake --build build
   ```

3. Launch it from project root:
   ```bash
   ./build/bin/main
   ```

## Styling

### Pixelated kamon

Change the pixelated kamon version using this command from ImageMagick:

```bash
magick assets/kamon.png -scale 160x160 -alpha off -depth 8 -colors 2 -scale 512x512 -filter point assets/kamon_pixelated.png
```

## Development

```bash
strip -x build/bin/main
```
-x option on macOS removes all non-global symbols, significantly shrinking size while still allowing a minimal backtrace.

```bash
find . -type f \( -name "*.cpp" -o -name "*.h" \) ! -name "argparse.hpp" -exec clang-format -i {} \;
```
