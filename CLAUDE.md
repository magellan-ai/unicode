# Project: unicode

C extension gem for Unicode normalization, case mapping, and character properties. Magellan's maintained fork of the
`unicode` gem by Yoshida Masato (forked from `blackwinter/unicode`).

# Ruby Environment

This project uses Ruby 4.0.6. However, note that Ruby 3.3 (currently 3.3.13) is LTS until the beginning of 2027, so
Ruby 3.3 and 3.4 should still be supported. CI runs the specs on 3.3, 3.4, and 4.0.

Developers use either RVM or rbenv; detect which is available and set up accordingly.

Before running any Ruby, bundle, rake, or rspec commands, always prefix with the appropriate setup:

**RVM** (if `~/.rvm/scripts/rvm` exists):
```
source ~/.rvm/scripts/rvm && rvm use ruby-4.0.6@unicode &&
```

**rbenv** (if `rbenv` is on PATH): no prefix needed — rbenv activates automatically via `.ruby-version`. Just run commands directly.

Examples (shown with RVM prefix — omit the prefix for rbenv):
- `source ~/.rvm/scripts/rvm && rvm use ruby-4.0.6@unicode && bundle exec rake` (compiles the extension, then runs the specs)
- `source ~/.rvm/scripts/rvm && rvm use ruby-4.0.6@unicode && bundle exec rubocop`
- `source ~/.rvm/scripts/rvm && rvm use ruby-4.0.6@unicode && bundle exec rake compile_commands` (writes the gitignored `compile_commands.json`, so clangd finds the Ruby headers; rerun after switching Rubies)

## Conventions

- Follow TDD: write specs first, verify they fail for the expected reason, then write the implementation to make them pass. Don't batch spec + implementation together and run once.
- **Recompile after every C change.** `bundle exec rspec` loads whatever binary is in `lib/unicode/`, so it silently tests a stale build. Run `bundle exec rake compile` first, or just `bundle exec rake`.
- **The Unicode data is pinned to 6.2.** Don't regenerate `ext/unicode/unidata.map` from newer files: it changes the output callers depend on (some hash or store normalized text). `tools/mkunidata.rb` reproduces the committed file byte for byte from the 6.2.0 data files (see `tools/README`), and `spec/unidata_generator_spec.rb` checks that it still does, so a change to the generator that alters its output fails the suite.
- Specs are split by concern (`spec/<concern>_spec.rb`), not by class. Pending specs record known C bugs; when a fix makes one pass, RSpec reports it, and you remove the `pending`.
- In specs, write ambiguous Unicode as `\u` escapes: combining marks, anything with a canonical or compatibility decomposition, Hangul, and invisible or private-use characters. Plain literals are fine for the rest (`'ß'`, `'あいう'`). `Style/WordArray` is disabled because its autocorrect turns escapes back into literals.
- Specs must run on Ruby 3.3, so don't use the `it` block parameter (3.4+).
- **Writing `\u` escapes from an agent:** tool inputs decode `\uXXXX` into the literal character before it reaches the file, and a doubled backslash isn't collapsed either. Generate escapes with a script (a backslash from `92.chr`), then check the bytes with `od -c`.

## Working on the C extension

Code at the Ruby/C boundary can compile, link, and pass every happy-path spec while being wrong in ways that only show up under GC pressure, on an exception, or under load, and then it corrupts unrelated objects instead of raising. That is how plausible-looking extension code fails, so treat every change to `ext/unicode/*.c` as high-risk:

- **Write the contract before the code**, as a comment at the boundary: who owns each pointer and `VALUE`, what can raise, what can allocate (and so trigger GC), and whether the GVL is held.
- **Say so when unsure.** "I don't know whether this call can allocate or raise" beats writing the call and hoping. When in doubt, assume it can: most of the Ruby C API can do both, `rb_hash_aref` included (it calls `#default` on a miss).
- **A `VALUE` kept in a C static must be a registered GC root before the next allocation.** Register it with `rb_global_variable` right after assigning it; an allocation in between can collect it, leaving the static pointing at whatever reuses the slot. Better still, keep lookup data in plain C structures instead of Ruby objects, as the character tables here are (`unidata[]`, `bmp_index`, `composition_pairs`).
- **Ruby raises by `longjmp`, which skips your C cleanup.** Every entry point keeps its buffers in a `struct call` on its stack and runs its work through `call_with_buffers`, whose `rb_ensure` handler frees them however the work exits; add new entry points the same way. Helpers never heap-allocate temporaries: mappings decode into stack arrays of `MAPPING_CAPACITY`.
- **Allocate buffers with `buffer_malloc`/`buffer_realloc`/`buffer_free` (`allocation.c`), never `malloc`.** They wrap `ruby_xmalloc` and friends, so they raise `NoMemoryError` instead of returning `NULL` and count toward GC malloc pressure, and they're what fault injection hooks. Because they can run the GC, any `VALUE` you decode from must stay reachable for the whole call: keep it in the `struct call`.
- **Pair `RSTRING_PTR` with `RSTRING_LEN`, and never hold the pointer across a Ruby allocation.** Use `RB_GC_GUARD` on a `VALUE` whose only remaining use is a pointer derived from it.
- **Check every decode and allocation result.** `WStr_allocWithUTF8L` returns `NULL` on invalid UTF-8 and its callers ignore that, so invalid input would be silently truncated. Entry points validate their arguments with `utf8_argument` before decoding, which keeps that path unreachable; route every new entry point through it.
- **GVL:** inside `rb_thread_call_without_gvl`, never touch a `VALUE` or call the Ruby API. Mark every release and reacquire explicitly. (Nothing here releases the GVL today.)
- **The extension is declared Ractor-safe** (`rb_ext_ractor_safe(true)`, first thing in `Init_unicode_native`, before any method is defined). Keep it true: no mutable C globals outside `Init`, and only shareable objects (static Symbols, frozen data) kept in or returned from shared state. `spec/ractor_spec.rb` runs the entry points from several Ractors at once.
- **Only `Init_unicode_native` is exported.** The extension builds with `-fvisibility=hidden`, so internal functions can't collide with another library's symbols in Ruby's shared namespace; `spec/exported_symbols_spec.rb` enforces it. Don't mark anything else `RUBY_FUNC_EXPORTED`.
- **Keep the C small.** Push logic into Ruby unless it's a hot path or genuinely needs native facilities.
- **Test under pressure, not just the happy path.** Keep `spec/garbage_collection_spec.rb` green (`GC.stress` while loading, then a full compaction), add a `GC.stress` case for any new allocation path, and run `bundle exec rake spec:sanitized` for every C change. It rebuilds the extension with AddressSanitizer and UndefinedBehaviorSanitizer, runs the suite with the ASan runtime preloaded, then clobbers the instrumented build; CI runs it too. It needs Linux and gcc, so on macOS run it in the CI image: `docker run --rm -v "$PWD":/src:ro cimg/ruby:4.0 bash -lc 'mkdir -p ~/project && cp -R /src/. ~/project/ && cd ~/project && bundle install --quiet && bundle exec rake spec:sanitized'`.
- **Out-of-memory paths are tested by fault injection.** The sanitizer build defines `UNICODE_FAULT_INJECTION`, which adds `Unicode::FaultInjection`: `fail_allocation(n)` makes the nth buffer allocation from now raise `NoMemoryError`, and `live_buffers` counts the buffers still allocated. `spec/allocation_failure_spec.rb` fails every allocation of every entry point in turn and checks that nothing leaks; add new entry points to `spec/support/allocation_failure_sweep.rb`. Don't reach for LeakSanitizer instead: it doesn't report leaked `ruby_xmalloc` memory here, even a block leaked deliberately.
- **Under the sanitizer build, Ruby threads crash.** With the ASan runtime preloaded into the uninstrumented Ruby, even `Thread.new {}.join` aborts with `Failed to munmap`, so specs must not create threads; that includes `Open3`, which reads output on threads. Run subprocesses through `run_ruby` (`spec/support/subprocess_helpers.rb`), which uses `IO.popen`. SimpleCov reports through threads too, so `spec_helper.rb` skips it when `UNICODE_SANITIZE` is set, and Ractors run on threads, so `spec/ractor_spec.rb` is skipped then as well.
