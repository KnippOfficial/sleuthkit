#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

POOLDIR=$1

if [[ -z ${POOLDIR} ]]
then
  echo "Usage $0 <pooldir>"
  exit 1
fi


mkdir -p $POOLDIR

cd $POOLDIR
dd if=/dev/zero of=test1.img bs=1M count=1000
dd if=/dev/zero of=test2.img bs=1M count=1000

zpool create ztest $POOLDIR/test1.img $POOLDIR/test2.img

mkdir -p /ztest/a/b
zfs create ztest/d
echo "aaaa" > /ztest/a/a.txt
echo "eeee" > /ztest/a/e.txt
echo "bbbb" > /ztest/a/b/b.txt
echo "cccc" > /ztest/a/b/c.txt
echo "dddd" > /ztest/d/d.txt

zpool export ztest
