#! /usr/bin/env bash

# simple driver for t_mpmcq

u=`uname`

np=2
nc=2
file=/tmp/x
obj=dbg

for a in $*; do
    case $a in
        np=*)
            np=${a##np=}
            ;;
        p=*)
            np=${a##p=}
            ;;

        nc=*)
            nc=${a##nc=}
            ;;
        c=*)
            nc=${a##c=}
            ;;

        o=*)
            file=${a##o=}
            ;;

        rel=*|rel)
            obj=rel
            ;;


        *)
            ;;

    esac
done

exe=${u}_objs_${obj}/t_mpmcq

echo "$exe $np $nc > $file"
$exe $np $nc >| $file
head -10 $file
