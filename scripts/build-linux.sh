#!/bin/sh
# Build the game on Linux, without root privileges.
#
#   ./scripts/build-linux.sh
#   ./scripts/build-linux.sh --build-type Release --jobs 8
#   ISO_DEPS=/opt/iso-deps ./scripts/build-linux.sh
#
# The script is idempotent. It
#
#   1. makes sure the X11/OpenGL/UDev *headers* are available. When they are not
#      installed system wide it extracts the missing Fedora -devel RPMs into a
#      private sysroot (dnf + rpm2cpio, no sudo),
#   2. builds SFML 3 (shared libraries) into a private prefix, unless a system
#      SFML 3 is already visible to pkg-config,
#   3. configures, builds and tests this project against that SFML.
#
# Everything it creates lives under $ISO_DEPS (default: ~/.cache/isometric-sfml3)
# and ./build, so `rm -rf` undoes all of it.
#
# Environment:
#   ISO_DEPS          cache directory for the sysroot + SFML build
#   ISO_SYSROOT       reuse an existing -devel sysroot instead of fetching one
#   ISO_SFML_PREFIX   reuse an existing SFML 3 installation
#   ISO_SFML_VERSION  SFML release to build (default: 3.1.0)
#   ISO_BUILD_TYPE    CMake build type (default: Debug)
#   ISO_JOBS          parallel compile jobs (default: number of CPUs)
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEPS=${ISO_DEPS:-${XDG_CACHE_HOME:-$HOME/.cache}/isometric-sfml3}
SYSROOT=${ISO_SYSROOT:-$DEPS/sysroot}
SFML_VERSION=${ISO_SFML_VERSION:-3.1.0}
# Version the install prefix so upgrading the default above builds a fresh SDK
# instead of silently reusing the previously cached release.
SFML_PREFIX=${ISO_SFML_PREFIX:-$DEPS/sfml3-$SFML_VERSION}
BUILD_TYPE=${ISO_BUILD_TYPE:-Debug}
JOBS=${ISO_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}

# RPMs that provide the headers SFML and this project need. Only consulted when
# the headers are missing and the host is Fedora-like.
RPM_PACKAGES="libXrandr-devel libXi-devel libXcursor-devel libXfixes-devel
libXrender-devel mesa-libGL-devel libglvnd-devel systemd-devel"

say() { printf '\033[1m==> %s\033[0m\n' "$*"; }
warn() { printf '\033[1;33mwarning: %s\033[0m\n' "$*" >&2; }
die() { printf '\033[1;31merror: %s\033[0m\n' "$*" >&2; exit 1; }

while [ $# -gt 0 ]; do
    case $1 in
    --build-type | --jobs)
        [ $# -ge 2 ] || die "$1 needs a value"
        if [ "$1" = "--build-type" ]; then BUILD_TYPE=$2; else JOBS=$2; fi
        shift 2
        ;;
    --clean) rm -rf "$ROOT/build"; shift ;;
    -h | --help)
        awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"
        exit 0
        ;;
    *) die "unknown argument: $1" ;;
    esac
done

for tool in cmake ninja; do
    command -v "$tool" >/dev/null 2>&1 || die "$tool not found (needed to build)"
done

have_header() {
    # Headers come from two places: whatever is installed system wide and the
    # private sysroot holding the -devel RPMs this machine is missing.
    [ -f "$SYSROOT/usr/include/$1" ] || [ -f "/usr/include/$1" ]
}

headers_present() {
    for header in X11/Xlib.h X11/extensions/Xrandr.h X11/extensions/XInput2.h \
        X11/Xcursor/Xcursor.h GL/gl.h libudev.h; do
        have_header "$header" || return 1
    done
}

fetch_headers() {
    say "Fetching X11/GL/UDev headers into $SYSROOT"
    command -v dnf >/dev/null 2>&1 || die "dnf not found; install these packages
    with your package manager and re-run: $RPM_PACKAGES"
    command -v rpm2cpio >/dev/null 2>&1 || die "rpm2cpio not found"
    command -v cpio >/dev/null 2>&1 || die "cpio not found"

    mkdir -p "$DEPS/rpms" "$SYSROOT"
    # shellcheck disable=SC2086
    dnf download --skip-unavailable --destdir="$DEPS/rpms" $RPM_PACKAGES \
        >"$DEPS/dnf.log" 2>&1 || die "dnf download failed, see $DEPS/dnf.log"

    for rpm in "$DEPS"/rpms/*.x86_64.rpm; do
        [ -e "$rpm" ] || continue
        rpm2cpio "$rpm" | (cd "$SYSROOT" && cpio -idmu --quiet)
    done

    # The -devel RPMs ship `libfoo.so -> libfoo.so.1`, and rpm2cpio does not
    # bring the runtime package that owns the versioned file. Point those
    # links at the copy the distribution already installed, otherwise
    # find_library()/ld reject them.
    for link in "$SYSROOT"/usr/lib64/*.so; do
        [ -L "$link" ] || continue
        if [ -e "$link" ]; then
            continue # the versioned file came with the same RPM, nothing to do
        fi
        target=$(readlink "$link")
        case $target in
        /*) continue ;; # absolute link, already resolved by the RPM
        esac
        if [ -e "/usr/lib64/$target" ]; then
            ln -sfn "/usr/lib64/$target" "$link"
        else
            warn "cannot resolve $link -> $target"
        fi
    done
}

system_sfml_is_v3() {
    command -v pkg-config >/dev/null 2>&1 || return 1
    version=$(pkg-config --modversion sfml-graphics 2>/dev/null) || return 1
    case $version in 3.*) return 0 ;; *) return 1 ;; esac
}

# Prints the version recorded in an SFML installation's CMake package file, or
# nothing when there is no usable installation in $1.
sfml_prefix_version() {
    for config in "$1/lib64/cmake/SFML/SFMLConfigVersion.cmake" \
        "$1/lib/cmake/SFML/SFMLConfigVersion.cmake"; do
        [ -f "$config" ] || continue
        sed -n 's/^[[:space:]]*set(PACKAGE_VERSION "\(.*\)")/\1/p' "$config" | head -n 1
        return 0
    done
    return 0
}

build_sfml() {
    say "Building SFML $SFML_VERSION into $SFML_PREFIX"
    source_dir=$DEPS/src/SFML-$SFML_VERSION
    if [ ! -f "$source_dir/CMakeLists.txt" ]; then
        mkdir -p "$DEPS/src"
        url="https://github.com/SFML/SFML/archive/refs/tags/$SFML_VERSION.tar.gz"
        say "Downloading $url"
        if command -v curl >/dev/null 2>&1; then
            curl -fsSL "$url" | tar xz -C "$DEPS/src"
        else
            wget -qO- "$url" | tar xz -C "$DEPS/src"
        fi
    fi

    cmake -S "$source_dir" -B "$source_dir/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$SFML_PREFIX" \
        -DBUILD_SHARED_LIBS=ON \
        -DSFML_BUILD_AUDIO=OFF \
        -DSFML_BUILD_NETWORK=OFF \
        -DSFML_BUILD_EXAMPLES=OFF \
        -DSFML_BUILD_DOC=OFF \
        -DCMAKE_INCLUDE_PATH="$SYSROOT/usr/include" \
        -DCMAKE_LIBRARY_PATH="$SYSROOT/usr/lib64" \
        -DCMAKE_PREFIX_PATH="$SYSROOT"
    cmake --build "$source_dir/build" -j "$JOBS"
    cmake --install "$source_dir/build"
}

# --- 1. headers -----------------------------------------------------------
if headers_present; then
    say "Reusing X11/GL/UDev headers from $SYSROOT"
else
    fetch_headers
    headers_present || die "headers still missing in $SYSROOT"
fi

# --- 2. SFML 3 ------------------------------------------------------------
sfml_prefix=""
if system_sfml_is_v3; then
    say "Using the system SFML 3 ($(pkg-config --modversion sfml-graphics))"
else
    cached_version=$(sfml_prefix_version "$SFML_PREFIX")
    if [ "$cached_version" = "$SFML_VERSION" ]; then
        say "Reusing SFML $cached_version from $SFML_PREFIX"
    else
        if [ -n "$cached_version" ]; then
            warn "ignoring cached SFML $cached_version in $SFML_PREFIX"
        fi
        build_sfml
    fi
    sfml_prefix=$SFML_PREFIX
fi

# --- 3. the game ----------------------------------------------------------
say "Configuring the project ($BUILD_TYPE)"
set -- "-DCMAKE_BUILD_TYPE=$BUILD_TYPE" \
    "-DCMAKE_INCLUDE_PATH=$SYSROOT/usr/include" \
    "-DCMAKE_LIBRARY_PATH=$SYSROOT/usr/lib64" \
    "-USFML_DIR" # drop a cached SFML location when the version changes
if [ -n "$sfml_prefix" ]; then
    set -- "$@" "-DCMAKE_PREFIX_PATH=$sfml_prefix"
fi

cmake -S "$ROOT" -B "$ROOT/build" -G Ninja "$@"

say "Building"
cmake --build "$ROOT/build" -j "$JOBS"

say "Running the headless test suite"
ctest --test-dir "$ROOT/build" --output-on-failure

cat <<EOF

$(printf '\033[1;32mDone.\033[0m')
  game   : $ROOT/build/IsometricGame
  tests  : ctest --test-dir $ROOT/build --output-on-failure

The game needs an X display. Without one, run it in a virtual X server:

  Xvfb :99 -screen 0 1920x1080x24 &
  DISPLAY=:99 $ROOT/build/IsometricGame

EOF
