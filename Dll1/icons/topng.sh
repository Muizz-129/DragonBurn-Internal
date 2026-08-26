#!/bin/bash

# Output width (optional)
WIDTH=1024

for file in *.svg; do
    name="${file%.svg}"
    echo "Converting $file -> ${name}.png"
    
    inkscape "$file" \
        --export-type=png \
        --export-filename="${name}.png" \
        --export-width=$WIDTH
done

echo "Done."
