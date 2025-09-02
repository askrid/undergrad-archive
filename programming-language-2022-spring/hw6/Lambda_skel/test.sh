for i in ./examples/*.l; do
    [ -f "$i" ] || break
    echo "\n"
    ./run $i
done