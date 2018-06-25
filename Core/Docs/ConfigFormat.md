# Config file format

Line comments start with `#`.

Blank lines are ignored.

Every other line must either be a section header or a kv-pair.

## Section header
`[<sectionName>]`. `[]` block may be preceded and/or followed by whitespace.

Sets the current section to be `<sectionName>`. Whitespace is not allowed in a section name.

kv-pair keys encountered after a section header will be named `<sectionName>.<key>` in the `Config`,
atleast until another section header is encountered. The default section header has a blank name,
i.e. keys are named '.<key>' by default. Using an empty `[]` block will reset the section name to the default.

## kv-pair
`<key> = <value>`

###`<key>`
An identifier made of any printable characters that are not `=` or whitespace/line breaks.
Whitespace between the start of the line and `<key>` and between `<key>` and the `=` is ignored.

###`<value>`
Whitespace between the `=` and the start of the value and between the end of the value and the line break is ignored.

One of:

- `"<string>"` or `'<string>'`; maps to `ConfigValue::String`.  
  Line breaks are allowed in strings and are included in the strings themselves.

- Integer; maps to `ConfigValue::I64`.  
  Can start with a `+` or `-` sign. `_` characters in the number are ignored.

- Float; maps to `ConfigValue::F64`.  
  Same as integer, but contains a `.` (decimal separator) character somewhere.

- Boolean; maps to `ConfigValue::Boolean`.  
  Either (`T` | `t`) or (`F` | `f`)
