#!/bin/bash
set -e
src=$(cd "$(dirname "$0")/.." && pwd)
g++ -std=c++17 -O2 "$src/tools/gpu-light/main.cpp" -o "${1:-gpu-light}" $(sdl2-config --cflags --libs) -lGL
