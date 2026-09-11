# TagTime

This is a simple terminal utility to track durations of tasks. 
Tasks can be tagged with various tags so they can be queried by them later.

## TODOs

### MEM Rework 

- [ ] one instance where platform local temp mem is used for persistent storage
      of a string
- [ ] when scoped arena is created of an arena it should not be possible to use 
      the normal arena for allocations
- [ ] restructure mem_arena API
      renaming by having the subject first e.g.: mem_arena_create

### General

- [ ] bug in cli parser, does not stop upon - in multiple arguments. Intentional?
- [ ] further related tags when querying entries
- [ ] sort tags by last used, etc...


## Features

- time tracking entry can be tagged with 1 to n tags
- tracked times can be queried by tags either outputting all recorded entries
    or a total sum
- times and tags are stored in a given file
- CLI interface 
- basic editing of entries (deleting a record, add / remove a tag from an entry)

### Time Tracking Entries

Duration stored in minutes (32 or 64 bit). 
A Timestamp for the entry is stored (`time()`) (64 bit).

This results to either 96 or 128 bit per entry. This means 1 MB of storage 
can store either 10922 or 8192 entries.




