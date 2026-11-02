# unicode

Unicode normalization, case mapping, and character properties for Ruby, as a C extension.

This is [Magellan AI](https://www.magellan.ai)'s maintained fork of the [`unicode`](https://rubygems.org/gems/unicode)
gem, written by Yoshida Masato and previously packaged by [blackwinter](https://github.com/blackwinter/unicode). We
keep it working on current Ruby: a real test suite, modern packaging, and fixes to the C extension. The original
author's site is no longer online, but its last version is
[archived on the Wayback Machine](https://web.archive.org/web/20130509170942/http://www.yoshidam.net/Ruby.html#unicode).

## Should you use it?

Honestly, probably not, particularly for new code. Ruby's `String#unicode_normalize`, `#downcase`, `#upcase`, and
`#capitalize` work, and with newer Unicode data; and `String#grapheme_clusters` supersedes `text_elements`. This gem is
for code that already depends on the existing gem's exact output, and for the few things Ruby doesn't offer:
`decompose_safe`, `compose` on its own, and terminal display `width`.

## Installation

```ruby
gem 'unicode', github: 'magellan-ai/unicode'
```

It needs Ruby 3.3 or newer and a C compiler.

## Usage

Every function is a module function on `Unicode`. Each takes a `String` and returns a new UTF-8 string (or an array,
or an integer). UTF-8 and US-ASCII strings are read as UTF-8; input in any other encoding is transcoded to UTF-8 first.
Bytes that aren't valid UTF-8 raise `ArgumentError` (as `String#unicode_normalize` does), and anything that isn't a
`String` raises `TypeError`. Every function is safe to call from any Ractor.

### Normalization

| Function           | Aliases                        | Result                                                |
| ------------------ | ------------------------------ | ----------------------------------------------------- |
| `decompose`        | `normalize_D`, `nfd`           | NFD                                                   |
| `decompose_compat` | `normalize_KD`, `nfkd`         | NFKD                                                  |
| `normalize_C`      | `nfc`                          | NFC                                                   |
| `normalize_KC`     | `nfkc`                         | NFKC                                                  |
| `decompose_safe`   | `normalize_D_safe`, `nfd_safe` | NFD, except that composition exclusions stay composed |
| `normalize_C_safe` | `nfc_safe`                     | `decompose_safe`, then `compose`                      |
| `compose`          |                                | Canonical composition of already-decomposed input     |

```ruby
Unicode.nfd("\u00E9")             # => "e\u0301"
Unicode.nfc("e\u0301")            # => "\u00E9"
Unicode.nfkd("\uFB03")            # => "ffi"
Unicode.nfkc("\uFF76\uFF9E")      # => "\u30AC"

# ANGSTROM SIGN is a composition exclusion:
Unicode.nfd("\u212B")             # => "A\u030A"
Unicode.decompose_safe("\u212B")  # => "\u212B"
```

### Comparison

`strcmp` and `strcmp_compat` compare the NFD and NFKD forms of two strings, codepoint by codepoint. Like `<=>`, they
return `-1`, `0`, or `1`, and `0` means the strings are equivalent.

```ruby
Unicode.strcmp("\u00E9", "e\u0301")              # => 0
Unicode.strcmp_compat("\uFF76\uFF9E", "\u30AC")  # => 0
Unicode.strcmp('a', 'b')                         # => -1
```

### Case mapping

`upcase`, `downcase`, and `capitalize` apply the unconditional mappings from `SpecialCasing.txt`, but no
language- or context-sensitive ones (so no Greek final sigma, and no Turkish dotless i).

```ruby
Unicode.upcase('straße')          # => "STRASSE"
Unicode.downcase('ΣΑΣ')           # => "σασ"
Unicode.capitalize("\u01C6EMAL")  # => "\u01C5emal"
```

### Character properties

`categories` and `abbr_categories` return each character's general category, long or abbreviated. `text_elements`
splits a string into user-perceived characters: a base character with its combining marks, or a Hangul syllable.
All three yield to a block instead, if given one, and then return the string.

```ruby
Unicode.abbr_categories('A1 ')     # => [:Lu, :Nd, :Zs]
Unicode.categories('A')            # => [:Uppercase_Letter]
Unicode.text_elements("e\u0301x")  # => ["e\u0301", "x"]
```

`width(string, cjk = false)` estimates how many columns a string takes on a fixed-pitch terminal, based on Markus
Kuhn's `wcwidth`. East Asian ambiguous characters count as wide only when `cjk` is true, and a string containing a
control character returns `-1`.

```ruby
Unicode.width('abc')           # => 3
Unicode.width('あいう')           # => 6
Unicode.width("\u03B1")        # => 1
Unicode.width("\u03B1", true)  # => 2
Unicode.width("a\tb")          # => -1
```

## Unicode version

The character data is **Unicode 6.2**, and it stays pinned there on purpose: updating it changes the output for some
characters, and callers that hash or store normalized text depend on it staying the same. Characters added after 6.2
are treated as unassigned, so they come back unchanged and their category is `:Cn`. For the exact differences from
Ruby's own (newer) implementation, see [`spec/ruby_parity_spec.rb`](spec/ruby_parity_spec.rb).

## Development

```sh
bundle install
bundle exec rake                     # compiles the extension and runs the specs
bundle exec rubocop
bundle exec rake spec:sanitized      # the specs under AddressSanitizer and UndefinedBehaviorSanitizer (Linux)
bundle exec rake compile_commands    # lets clangd-based editors find the Ruby headers
```

Besides per-function specs, the suite runs the Unicode 6.2 `NormalizationTest.txt` conformance tests, compares every
codepoint against Ruby's own implementation, and checks the extension under GC stress and from several Ractors. The
sanitizer build also makes each memory allocation fail in turn, to check that nothing leaks or corrupts memory when one
does. [`CONTRIBUTING.md`](CONTRIBUTING.md) covers what a pull request needs. `tools/mkunidata.rb` regenerates
`ext/unicode/unidata.map` from the Unicode Character Database; see [`tools/README`](tools/README).

## License

Copyright Yoshida Masato. Distributed under the same terms as Ruby: the 2-clause BSD License, or Ruby's own
conditions. The Unicode Character Database data it's built from is under the Unicode License. See
[`LICENSE`](LICENSE) for all three.

## History

**0.5.0** (this fork):

- Requires Ruby 3.3 or newer. The JRuby version and the prebuilt Windows gems are gone.
- Looks characters up in plain C tables instead of Ruby hashes. That fixes a failure in which a process could boot
  with its tables broken, so that every call raised errors such as
  `undefined method 'default' for an instance of String` until the process exited. It also makes `downcase` and
  normalization faster.
- Bytes that aren't valid UTF-8 raise `ArgumentError` instead of being silently dropped, along with the rest of the
  string.
- `strcmp` and `strcmp_compat` compare past embedded NULs and return `-1`, `0`, or `1` instead of a byte difference.
- Running out of memory raises `NoMemoryError` instead of corrupting memory, and no call leaks memory when it raises.
- Every function is safe to call from any Ractor.
- `Unicode::VERSION` now matches the gem's version.

Upstream releases by Yoshida Masato:

- **0.4.4** (2013-02-07): updated the data to Unicode 6.2
- **0.4.3** (2012-08-08): added `categories`, `text_elements`, and `width`
- **0.4.2** (2012-02-29): added `decompose_safe`
- **0.4.1** (2012-02-03): updated the data to Unicode 6.1
- **0.4.0** (2010-10-14): fixed the composition algorithm; Unicode 6.0
- **0.3.0** (2010-02-26): fixed a `capitalize` bug; added SpecialCasing support
- **0.2.0** (2009-12-29): Ruby 1.9.1 support; Unicode 5.2
- **0.1.2** (2005-09-10): Unicode 4.1.0
- **0.1.1** (2004-08-26): Unicode 4.0.1
- **0.1** (1999-11-23): first release
