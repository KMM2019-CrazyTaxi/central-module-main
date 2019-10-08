CCX=clang++
CCXFLAGS = -std=c++17

SRCDIR  = ./src
OBJSDIR = ./build
DEPDIR	= ./include
GPUDIR = ./QPULib/Lib

# Find all subdirectories
INCLUDES = $(shell find $(SRCDIR) -type d | sed s/^/-I/)
INCLUDES += $(shell find $(GPUDIR) -type d | sed s/^/-I/)

# Get all headers and sources from source directory
HEADERS = $(shell find $(SRCDIR) -type f -name '*.h')
SOURCES = $(shell find $(SRCDIR) -type f -name '*.cpp')

# Generate all objects and matching objects without directory path
OBJECTS  = $(SOURCES:$(SRCDIR)%.cpp=$(OBJSDIR)%.o)
OBJECTS_NO_PATH = $(foreach obj, $(OBJECTS), $(OBJSDIR)/$(notdir $(obj)))

project: $(OBJECTS)
	$(CCX) $(CCXFLAGS) $(OBJECTS_NO_PATH) -o project.out

$(OBJECTS): $(OBJSDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS)
	$(CCX) $(CCXFLAGS) $(INCLUDES) -c $< -o $(OBJSDIR)/$(@F)

clean:
	rm -f project.out
	find $(OBJSDIR)/ -name '*.o' -delete
	find $(DEPDIR)/ -name '*.h.gch' -delete
	rm -r project.dSYM

run:
	@echo "------------------------------ Compiling project... ------------------------------"		
	make
	@echo "------------------------------ Running project... --------------------------------"
	./project.out