# Frequency Prefix Automaton – Text Autocomplete (C)

A command-line text generator in C that learns from input sentences and autocompletes prompts with the most frequent continuation.

## How it works
- **Stage 0:** Builds a frequency prefix tree (trie) from input statements, counting how often each state is traversed.
- **Stage 1:** For each prompt, follows the tree and greedily appends the most frequent next characters (max 37 chars).
- **Stage 2:** Compresses single-child chains into multi-character arcs (Patricia-trie style) using DFS, then reruns prompts on the compressed automaton.

## Concepts
Tries · Linked lists · DFS · Dynamic memory management · Greedy generation

## Build & Run
```bash
gcc -Wall -std=c99 -o autocomplete main.c
./autocomplete < input.txt
```
