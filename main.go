package main

import (
	"fmt"

	"github.com/dalygbarron/lambs/defs"
)

func main() {
	fmt.Println("Hello, World!")
	def := defs.MakeObjDef()
	def.AddChild("johnny", defs.MakeObjDef())
	fmt.Printf("%s\n", def)
}
