#!/bin/sh

src=$1
[ -r "$src" ] || { echo "Нет файла: $src" >&2; exit 1; }
out=$(grep -E '^[[:space:]]*(//|/\*|\*|%)' "$src" |
      sed -n 's/^.*Output:[[:space:]]*\([^[:space:]*]*\).*$/\1/p' | head -n 1)
[ -n "$out" ] || { echo "Нет комментария Output:" >&2; exit 2; }

dir=$(cd "$(dirname "$src")" && pwd)
src=$dir/$(basename "$src")

tmp= part=
trap 'rm -rf "$tmp" "$part"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 131' QUIT
trap 'exit 134' ABRT
trap 'exit 138' USR1
trap 'exit 140' USR2
trap 'exit 143' TERM

tmp=$(mktemp -d) || exit 4
export TMPDIR="$tmp"
cd "$tmp" || exit 4 

case $src in
    *.c)   cc  -o "$out" "$src" || exit 4 ;;
    *.cpp) c++ -o "$out" "$src" || exit 4 ;;
    *.tex) TEXINPUTS="$dir:" pdflatex -interaction=nonstopmode "$src" >/dev/null || exit 4
           mv "$(basename "$src" .tex).pdf" "$out" || exit 4 ;;
    *) echo "Неизвестный тип файла (нужны .c, .cpp, .tex)" >&2; exit 3 ;;
esac

part=$dir/.$out.$$
cp "$out" "$part" && mv "$part" "$dir/$out" || exit 4
echo "Готово: $dir/$out"