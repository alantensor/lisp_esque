CC = gcc
CFLAGS = -std=c99 -Wall -g
LDFLAGS = -lm

TARGET = mylisp

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
LEX_SOURCE = $(SRC_DIR)/lexer.l
BISON_SOURCE = $(SRC_DIR)/parser.y

LEX_GEN_C = $(SRC_DIR)/lexer.yy.c
BISON_GEN_C = $(SRC_DIR)/parser.tab.c
BISON_GEN_H = $(SRC_DIR)/parser.tab.h

OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
OBJECTS += $(OBJ_DIR)/lexer.yy.o $(OBJ_DIR)/parser.tab.o

EXECUTABLE = $(BIN_DIR)/$(TARGET)

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(BISON_GEN_H) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(OBJ_DIR)/lexer.yy.o: $(LEX_GEN_C) $(BISON_GEN_H) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(OBJ_DIR)/parser.tab.o: $(BISON_GEN_C) $(BISON_GEN_H) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(LEX_GEN_C): $(LEX_SOURCE)
	flex -o $@ $<

$(BISON_GEN_C) $(BISON_GEN_H): $(BISON_SOURCE)
	bison -d -o $(BISON_GEN_C) $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LEX_GEN_C) $(BISON_GEN_C) $(BISON_GEN_H) $(TARGET) *~ $(SRC_DIR)/*~ $(SRC_DIR)/*.yy.c $(SRC_DIR)/*.tab.c $(SRC_DIR)/*.tab.h

$(SRC_DIR):
	mkdir -p $(SRC_DIR)

$(OBJECTS): $(BISON_GEN_H)
