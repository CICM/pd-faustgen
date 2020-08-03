#! /bin/bash

# Copyright (c) 2020 Albert Gräf <aggraef@gmail.com>. Distributed under the
# MIT license, please check the toplevel LICENSE file for details.

# Create a self-contained distribution tarball from the source, including all
# the submodules.

# Invoke this in the toplevel directory as `./make-dist.sh`. It will leave the
# tarball as pd-faustgen-<version>.tar.gz in the toplevel directory. Temporary
# data is written to the pd-faustgen-<version> directory, which is
# automatically deleted afterwards. The version number is extracted from
# src/faustgen_tilde.c, so make sure to keep that up-to-date.

# NOTE: git and tar need to be installed to make this work. As a side-effect,
# all submodules will be checked out recursively, using `git submodule update
# --init --recursive`.

# This may need adjusting if the file is edited and the macro name changes.
version=$(grep "#define FAUSTGEN_VERSION_STR" src/faustgen_tilde.c | sed 's|^#define *FAUSTGEN_VERSION_STR *"\(.*\)".*|\1|')

# Distribution basename and name of the source tarball.
dist=pd-faustgen-$version
src=$dist.tar.gz

# Make sure that the submodules are initialized.
git submodule update --init --recursive

# List of submodules to include in the package. This requires git version
# 1.7.8 or later to work.
submodules=$(find . -mindepth 2 -name '.git' -type f -print | xargs grep -l "gitdir" | sed -e 's/^\.\///' -e 's/\.git$//')

# Remove any left-over temp directory and previous tarball.
rm -rf $dist $src
# Grab the main source.
git archive --format=tar.gz --prefix=$dist/ HEAD | tar xfz -
# Grab the submodules.
for x in $submodules; do (cd $dist && git -C ../$x archive --format=tar.gz --prefix=$x HEAD | tar xfz -); done

# Create the source tarball.
tar cfz $src $dist
rm -rf $dist
