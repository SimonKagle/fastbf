make
rm -rf results/*

for f in tests/*.bf; do
    s1=${f%.bf}
    s=${s1#tests/}
    if [ ! -f tests/$s.in ]; then
        ./bf $f > results/$s.res
    else
        cat tests/$s.in | ./bf $f > results/$s.res
    fi
    diff results/$s.res tests/$s.out
    if [ ! $? -eq 0 ]; then
        echo "File $f out different than expected"
    fi
done

exit 0