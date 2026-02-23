OPT=-O0
DEBUG=-g
LIBS=
FLAGS=-std=c99

BUILDDIR=./build
BINARY=$(BUILDDIR)/tagtime
INCLUDEDIRS=-I.

#.PHONY: buildandrun
#buildandrun: binary
#	$(BINARY)

.PHONY: binary
binary: $(BUILDDIR)
	gcc main.c $(OPT) $(DEBUG) $(LIBS) $(FLAGS) $(INCLUDEDIRS) -o $(BINARY) 

$(BUILDDIR):
	@mkdir $(BUILDDIR)
	@echo "*" > $(BUILDDIR)/.gitignore
	@echo "!.gitignore" >> $(BUILDDIR)/.gitignore

.PHONY : clean
clean: 
	rm -rf $(BUILDDIR)
	rm -rf .ccls-cache
	rm -f compile_commands.json
