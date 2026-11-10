package defs

import "fmt"

type ObjDef struct {
	children map[string]Def
}

func MakeObjDef() ObjDef {
	return ObjDef{
		children: map[string]Def{},
	}
}

func (d ObjDef) AddChild(name string, child Def) bool {
	_, ok := d.children[name]
	if ok {
		return false
	}
	d.children[name] = child
	return true
}

func (d ObjDef) GetValue(path Path) (string, error) {
	if path.Empty() {
		return "", fmt.Errorf("trying to extract value from object def")
	}
	next := path.Head()
	child, ok := d.children[next]
	if ok {
		return child.GetValue(path.Body())
	}
	return "", fmt.Errorf("No child named %s for %s", next, path)
}
