# Minimalistic JSON Parser for C

## Things to improve
- Implement string literal escape sequence
- Error handling is still really funky. Works fine but the diagnosis doesn't \
  work well as it stops at the first error it encountered \
  NOTE: I think look a head of 1 might fix this. But not a priority for now.
- Give user flexibility to use their own allocator, including the one that requires context
- Kinda used linked list too much in this, probably for the best... or not
