#!/bin/sh

src=$1
out=$(grep -E '^[[:space:]]*(//|/\*|\*|%)' "$src" |
      sed -n 's/^.*Output:[[:space:]]*\([^[:space:]*]*\).*$/\1/p' | head -n 1)
[ -n "$out" ] || { echo "Нет комментария Output:" >&2; exit 2; }

dir=$(cd "$(dirname "$src")" && pwd)
src=$dir/$(basename "$src")

tmp=$(mktemp -d) || exit 4
trap 'rm -rf "$tmp"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

export TMPDIR="$tmp" 

case $src in
    *.c)   cc  -o "$tmp/$out" "$src" || exit 4 ;;
    *.cpp) c++ -o "$tmp/$out" "$src" || exit 4 ;;
    *.tex)
        cd "$tmp" || exit 4
        pdflatex -interaction=nonstopmode "$src" >/dev/null || exit 4
        mv "$(basename "$src" .tex).pdf" "$out" ;;
    *) echo "Неизвестный тип файла" >&2; exit 3 ;;
esac

cp "$tmp/$out" "$dir/$out" || exit 4
echo "Готово: $dir/$out"