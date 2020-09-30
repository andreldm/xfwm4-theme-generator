CC=gcc
CFLAGS=-O2 -Wall -Wextra $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS=$(shell pkg-config --libs gtk+-3.0)
THEME_DIR = ~/.themes/Theme-Generator/xfwm4

PNG_FILES = 								\
	close-active.png						\
	close-inactive.png						\
	close-prelight.png						\
	close-pressed.png						\
	hide-active.png							\
	hide-inactive.png						\
	hide-prelight.png						\
	hide-pressed.png						\
	maximize-active.png						\
	maximize-inactive.png					\
	maximize-prelight.png					\
	maximize-pressed.png					\
	title-1-active.png						\
	title-1-inactive.png					\
	top-left-active.png						\
	top-left-inactive.png					\
	top-right-active.png					\
	top-right-inactive.png					\
	bottom-active.png						\
	bottom-inactive.png						\
	bottom-left-active.png					\
	bottom-left-inactive.png				\
	bottom-right-active.png					\
	bottom-right-inactive.png				\
	left-active.png							\
	left-inactive.png						\
	right-active.png						\
	right-inactive.png						\
	menu-active.png							\
	menu-inactive.png

generator: theme-generator.c
	$(CC) $(CFLAGS) theme-generator.c -o theme-generator $(LDFLAGS)

install:
	mkdir -p $(THEME_DIR)
	cp theme/themerc $(THEME_DIR)

	for png_file in $(PNG_FILES); do \
		image=$${png_file%.png}; \
		convert theme/$$png_file -alpha set +dither $(THEME_DIR)/$$image.xpm; \
	done

	for n in 2 3 4 5; do \
		cd $(THEME_DIR) && ln -s title-1-active.xpm title-$$n-active.xpm; \
		cd $(THEME_DIR) && ln -s title-1-inactive.xpm title-$$n-inactive.xpm; \
	done

set-theme:
	xfconf-query --channel xfwm4 --property /general/theme --set Default
	xfconf-query --channel xfwm4 --property /general/theme --set Theme-Generator

.SILENT: install set-theme
.PHONY: install set-theme
