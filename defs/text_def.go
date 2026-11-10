package defs

type TextDef struct {
	text string
}

func MakeTextDef(text string) TextDef {
	return TextDef{
		text: text,
	}
}

func (d *TextDef) GetValue(path Path) (string, error) {
	return d.text, nil
}
