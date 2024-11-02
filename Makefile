EMCC = emcc
SRCDIR = src
BUILDDIR = build
ASSETS_DIR = assets

# Source files
SOURCES_CPP = $(wildcard $(SRCDIR)/*.cpp)
SOURCES_C   = $(wildcard $(SRCDIR)/*.c)
SOURCES     = $(SOURCES_CPP) $(SOURCES_C)

# Object files
OBJECTS_CPP = $(SOURCES_CPP:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)
OBJECTS_C   = $(SOURCES_C:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
OBJECTS     = $(OBJECTS_CPP) $(OBJECTS_C)

# Preload files
PRELOAD = $(ASSETS_DIR)/texture_atlas.png

# Shell file - this is the template for index.html
SHELLFILE = shell.html

# Falgs
CFLAGS = -O3 -flto \
         -s USE_SDL=2 \
         -s USE_SDL_IMAGE=2 \
         -s SDL2_IMAGE_FORMATS='["png"]' \
         --preload-file $(PRELOAD)

# Output target
TARGET = index.html
OUTPUT = $(BUILDDIR)/$(TARGET)

.PHONY: all
all: $(OUTPUT)

# Compile C++ source files into object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(EMCC) -c $< -o $@ $(CFLAGS)

# Compile C source files into object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(EMCC) -c $< -o $@ $(CFLAGS)

# Link object files to final output
$(OUTPUT): $(OBJECTS) $(SHELLFILE)
	$(EMCC) $(OBJECTS) -o $@ $(CFLAGS) --shell-file $(SHELLFILE)

# Clean build
.PHONY: clean
clean:
	rm -rf $(BUILDDIR)