CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS =

SRCDIR = .
OBJDIR = obj
BINDIR = .

TARGET = social_network
ZIPFILE = 10_xlitvi02_xstepa77.zip

SRC = $(SRCDIR)/ims.c
OBJ = $(OBJDIR)/ims.o

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ): $(SRC)
	@mkdir -p $(OBJDIR) 
	$(CC) $(CFLAGS) -c $< -o $@

zip:
	@zip -j $(ZIPFILE) $(SRC) Makefile dokumentace.pdf
	@echo "Created archive: $(ZIPFILE)"

run: all
	@./$(BINDIR)/$(TARGET)
	@echo "Program executed successfully!"

clean:
	@rm -rf $(OBJDIR) 
	@rm -f $(BINDIR)/$(TARGET) 
	@rm -f $(ZIPFILE)
	@echo "All cleaned up!"

.PHONY: all clean zip run
