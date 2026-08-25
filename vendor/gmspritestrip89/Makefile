CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror

SA89_SRC = vendor/SpriteAsset89_v0.1.2/src/spriteasset89.c
GMSS_SRC = src/gmspritestrip89.c

all: test_gmspritestrip89

test_gmspritestrip89: $(SA89_SRC) $(GMSS_SRC) tests/test_gmspritestrip89.c
	$(CC) $(CFLAGS) -Iinclude -Ivendor/SpriteAsset89_v0.1.2/include $(SA89_SRC) $(GMSS_SRC) tests/test_gmspritestrip89.c -o $@

clean:
	rm -f test_gmspritestrip89
