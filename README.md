# Minimalistic JSON Parser for C

## Things to improve
- Test cases, heck this one is not even a test case
- Implement string literal escape sequence
- For now its parse only, no building JSON (yet? This fit my use case for now)
- Error handling is still really funky. Works fine but the diagnosis doesn't \
  work well as it only sees the current token \
  NOTE: Using a look a head of 1 might fix this. But not a priority for now.
- Give user flexibility to use their own allocator, including the one that requires context
