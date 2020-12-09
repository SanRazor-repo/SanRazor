# This file contains utility functions to build experiments. It is meant to be
# sourced into other scripts.

set -e

if ! which asap-clang > /dev/null; then
    echo "Please add asap-clang to path" >&2
    exit 1
fi

if [ -n "$ASAP_STATE_PATH" ]; then
    echo "ASAP_STATE_PATH already set! Please unset..." >&2
    exit 1
fi

export ASAN_OPTIONS="malloc_context_size=0:detect_leaks=0"

# Find the number of jobs to use in parallel make. Ninja kindly computes this
# for us, although I doubt developers consider help texts part of the stable
# public API ;-)
export N_JOBS="$( ninja --help 2>&1 | grep "run N jobs in parallel" | \
                  grep -o "default=[0-9]\+" | grep -o "[0-9]\+" )"

# Updates a repository
# Run this in a folder containing a checked-out repository. It will try to pull
# the latest changes from upstream.
update_repository() {
    local shelved=false

    # Check for git
    if [ -f .git/config ]; then
        git stash && git pull
        if [ -n "$( git stash list )" ]; then git stash pop; fi

    # Check for mercurial
    elif [ -f .hg/hgrc ]; then
        if hg shelve; then shelved=true; fi
        hg pull && hg up
        if [ "$shelved" = "true" ]; then hg unshelve; fi

    else
        echo "$(pwd) is neither a git nor an hg repository." >&2
        return 1
    fi
}

# Builds an initial build, using asap-clang
build_asap_initial() {
    local program="$1"
    local name="$2"
    shift; shift

    local target_dir="${program}-${name}-initial-build"
    local build_dir="${program}-${name}-build"

    # Don't rebuild if target folder exists
    [ -d "$target_dir" ] && return 0

    rm -rf "$build_dir"
    mkdir "$build_dir"

    (
        cd "$build_dir"
        asap-clang -asap-init
        export ASAP_STATE_PATH="$(pwd)/asap_state"

        # Build using the build command given as arguments
        "$@"
    )
    rsync -a --delete "$build_dir/" "$target_dir"
}

# Builds a coverage-instrumented version of the program.
build_asap_coverage() {
    local program="$1"
    local name="$2"
    shift; shift

    local target_dir="${program}-${name}-coverage-build"
    local source_dir="${program}-${name}-initial-build"
    local build_dir="${program}-${name}-build"

    # Don't rebuild if target folder exists
    [ -d "$target_dir" ] && return 0
    if ! [ -d "$source_dir" ]; then
        echo "Source folder \"$source_dir\" does not exist!" >&2
        exit 1
    fi

    rsync -a --delete "$source_dir/" "$build_dir"
    (
        cd "$build_dir"
        export ASAP_STATE_PATH="$(pwd)/asap_state"
        asap-clang -asap-coverage

        # Re-build using the build command given as arguments
        # This command should also run tests.
        "$@"
    )
    rsync -a --delete "$build_dir/" "$target_dir"
}

# Builds an optimized version of the program.
build_asap_optimized() {
    local program="$1"
    local name="$2"
    local optname="$3"
    local level="$4"
    shift; shift; shift; shift

    local target_dir="${program}-${name}-${optname}-build"
    local source_dir="${program}-${name}-coverage-build"
    local build_dir="${program}-${name}-build"

    # Don't rebuild if target folder exists
    [ -d "$target_dir" ] && return 0
    if ! [ -d "$source_dir" ]; then
        echo "Source folder \"$source_dir\" does not exist!" >&2
        exit 1
    fi

    rsync -a --delete "$source_dir/" "$build_dir"
    (
        cd "$build_dir"
        export ASAP_STATE_PATH="$(pwd)/asap_state"
        asap-clang -asap-optimize "$level"

        # Re-build using the build command given as arguments
        "$@"
    )
    rsync -a --delete "$build_dir/" "$target_dir"
}

# Runs benchmarks in a given folder
benchmark_asap() {
    local program="$1"
    local name="$2"
    shift; shift

    local benchmark_dir="${program}-${name}-build"
    if ! [ -d "$benchmark_dir" ]; then
        echo "Benchmark folder \"$benchmark_dir\" does not exist!" >&2
        exit 1
    fi

    (
        cd "$benchmark_dir"
        "$@"
    )
}

