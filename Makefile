#OBJS specifies which files to compile as part of the project
OBJS = main.c

#CC specifies which compiler we're using
CC = gcc

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
COMPILER_FLAGS = -w -g

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lSDL3 -lSDL3_image -lSDL3_mixer -lSDL3_ttf -lxml2
LINKER_FILES = deps/cJSON.c deps/sprite.c deps/player.c deps/asteroid.c deps/timer.c deps/bullet.c deps/powerup.c deps/button.c deps/score.c deps/init.c deps/draw.c

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = main

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FILES) $(LINKER_FLAGS) -o $(OBJ_NAME)
