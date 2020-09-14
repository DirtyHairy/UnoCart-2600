package main

import (
	"fmt"
	"io/ioutil"
	"os"
)

func main() {
	if len(os.Args) != 2 {
		println("usage: bus_dump_analyzer <dump.bin>")
		return
	}

	fpath := os.Args[1]

	var dumpfile *os.File
	var err error

	if dumpfile, err = os.Open(fpath); err != nil {
		fmt.Fprintf(os.Stderr, "unable to open file '%s'\n", fpath)
		return
	}

	var finfo os.FileInfo
	if finfo, err = dumpfile.Stat(); err != nil {
		fmt.Fprintf(os.Stderr, "unable to stat file '%s'\n", fpath)
		return
	}

	if finfo.IsDir() {
		fmt.Fprintf(os.Stderr, "'%s' is a directory\n", fpath)
		return
	}

	if finfo.Size() != 64*1024 {
		fmt.Fprintf(os.Stderr, "'%s' has wrong size\n", fpath)
		return
	}

	dumpfile.Close()

	var dump []byte
	if dump, err = ioutil.ReadFile(fpath); err != nil {
		fmt.Fprintf(os.Stderr, "unable to open file '%s'\n", fpath)
		return
	}

	initialRepeat := uint32(dump[0]) | uint32(dump[1])<<8 | uint32(dump[2])<<16 | uint32(dump[3])<<24
	initialWord := uint16(dump[4]) | uint16(dump[5])<<8
	fmt.Fprintf(os.Stdout, "0x%04x x %d\n", initialWord, initialRepeat)

	var value uint16 = uint16(dump[6]) | uint16(dump[7])<<8
	mult := 0

	for i := 3; i < 32*1024; i++ {
		var next uint16 = uint16(dump[2*i]) | uint16(dump[2*i+1])<<8

		if next == value {
			mult = mult + 1
		} else {
			fmt.Fprintf(os.Stdout, "0x%04x x %d\n", value, mult)

			mult = 1
		}

		value = next
	}

	fmt.Fprintf(os.Stdout, "0x%04x x %d\n", value, mult)
}
