#!/bin/sh

set -o errexit

cd "${0%/*}"
./generate_imgui_bindings.pl < ../imgui/imgui.h > imgui_iterator.h
