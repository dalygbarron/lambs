package defs

import (
	"fmt"
	"strings"
)

// Represents a path which is a bunch of textual tokens known as names seperated
// by . characters. Any other stuff like numbers or symbols is bad and unicode
// is not supported.
type Path struct {
	parts []string
}

// Validates that a list of names produces a valid path. Really just checking
// everything is a normal ASCII character.
func validateParts(parts []string) error {
	for _, p := range parts {
		if len(p) == 0 {
			return fmt.Errorf("empty name in path is invalid")
		}
		for _, r := range p {
			if (r < 'a' || r > 'z') && (r < 'A' || r > 'Z') {
				return fmt.Errorf("Name %s is no good in path", p)
			}
		}
	}
	return nil
}

// Takes a string of names seperated by . characters and turns it into a path
// object.
func MakePath(text string) (Path, error) {
	parts := strings.Split(text, ".")
	return Path{parts}, validateParts(parts)
}

// Gets the root of the path right now. If there is no root because it is empty
// then an empty string is returned.
func (p *Path) Head() string {
	if len(p.parts) > 0 {
		return p.parts[0]
	}
	return ""
}

// Returns a path which is the same as this one but without the first
func (p *Path) Body() Path {
	return Path{p.parts[1:]}
}

// Tells you if the path is empty.
func (p *Path) Empty() bool {
	return len(p.parts) == 0
}
