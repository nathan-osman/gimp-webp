# Predefined constants
CC      = gcc
TARGET  = file-webp
SRC_DIR = src
OBJ_DIR = obj
CFLAGS  = $(shell pkg-config --cflags gtk+-2.0 gimp-2.0 libwebp)
LFLAGS  = $(shell pkg-config --libs glib-2.0 gtk+-2.0 gimp-2.0 gimpui-2.0 libwebp)

# File definitions
SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES=$(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

$(TARGET): $(OBJ_DIR) $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $(TARGET) $(LFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_DIR):
	test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)

clean:
	test ! -d $(OBJ_DIR) || rm -rf $(OBJ_DIR)
	test ! -f $(TARGET) || rm $(TARGET)

install:
	gimptool-2.0 --install-bin $(TARGET)

uninstall:
	gimptool-2.0 --uninstall-bin $(TARGET)

.PHONY: clean install uninstall
