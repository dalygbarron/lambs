package defs

import "fmt"

type ListDef struct {
	children []Def
}

func MakeListDef() ListDef {
	return ListDef{
		children: []Def{},
	}
}

func (d *ListDef) AddChild(child Def) {
	d.children = append(d.children, child)
}

func (d ListDef) GetValue(path Path) (string, error) {
	return "", fmt.Errorf("Trying to get value ambiguously at %s", path)
}
