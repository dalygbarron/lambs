package defs

type Def interface {
	// Tries to get a string value at the given path which is considered to be
	// relative to this def node and can be dispatched to child nodes if they
	// exist and can be referenced unambiguously by the path provided.
	GetValue(path Path) (string, error)
}
