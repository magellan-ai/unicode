# Contributing

This is [Magellan AI](https://www.magellan.ai)'s maintained fork of the `unicode` gem. Report issues and open pull
requests here, at [magellan-ai/unicode](https://github.com/magellan-ai/unicode).

The library's original author, Yoshida Masato, published it from his own site, which is no longer online; its last
version is [archived on the Wayback Machine](https://web.archive.org/web/20130509170942/http://www.yoshidam.net/Ruby.html#unicode).
The domain has since passed to someone else, so don't follow links or send mail to `yoshidam.net`.

## Before you open a pull request

```sh
bundle exec rake          # compiles the extension and runs the specs
bundle exec rubocop
```

Changes to the C extension in `ext/unicode` also need to pass under AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
bundle exec rake spec:sanitized    # Linux with gcc; CLAUDE.md has the Docker command for macOS
```

[`CLAUDE.md`](CLAUDE.md) records the project's conventions and the rules for working on the C extension safely. It's
written for AI coding agents, but it's the most complete guide for people too.
