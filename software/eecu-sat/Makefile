
PROJ = eecu-sat

INIH_DIR  = ../lib/inih
INIH_SRC  = ${INIH_DIR}/ini.c
INIH_OBJ  = $(patsubst ${INIH_DIR}/%.c,build/lib/inih/%.o,${INIH_SRC})

NATSORT_DIR  = ../lib/natsort
NATSORT_SRC  = ${NATSORT_DIR}/strnatcmp.c
NATSORT_OBJ  = $(patsubst ${NATSORT_DIR}/%.c,build/lib/natsort/%.o,${NATSORT_SRC})

LIBS_OBJ = $(INIH_OBJ) $(NATSORT_OBJ)

INCLUDES  := -I ./ -I $(INIH_DIR) -I $(NATSORT_DIR)
LOCAL_SRC_C := main.c saleae.c session.c parsers.c error.c output.c output_analog.c output_srzip.c output_calibrate_linear_3p.c calib.c transform.c transform_calibrate_linear_3p.c trigger.c
SRC        = $(LOCAL_SRC_C) $(INIH_SRC) $(NATSORT_SRC)
EXOUTPUT   = $(PROJ)

LOCAL_OBJECTS     = $(patsubst %.c,build/%.o,${LOCAL_SRC_C})

# Append specific CFLAGS/LDFLAGS
CFLAGS_REL	 += -O3
LDFLAGS_REL	 += -Wl,-s
CFLAGS_DBG	 += -O1 -ggdb -DNDEBUG # -pg
LDFLAGS_DBG	 += # -pg

MANDATORY_CFLAGS=-fPIC -pedantic -Wall -Wextra -Wno-sign-compare `pkgconf --cflags libzip` `pkgconf --cflags glib-2.0` `pkgconf --cflags libsigrok` # -Wa,-ahl=$(@:.o=.s)
MANDATORY_LDFLAGS=`pkgconf --libs libzip` `pkgconf --libs glib-2.0` `pkgconf --libs libsigrok`

DEBUG := $(shell grep "^#define CONFIG_DEBUG" config.h)
ifeq ($(DEBUG),)
TARGET	:= RELEASE
TARGET_CFLAGS	+= $(MANDATORY_CFLAGS) $(CFLAGS_REL)
TARGET_LDFLAGS	+= $(MANDATORY_LDFLAGS) $(LDFLAGS_REL)
else
TARGET	:= DEBUG
TARGET_CFLAGS	+= $(MANDATORY_CFLAGS) $(CFLAGS_DBG)
TARGET_LDFLAGS	+= $(MANDATORY_LDFLAGS) $(LDFLAGS_DBG)
endif

COVERITY_SCAN_TOKEN := $(shell cat ../../.secrets.COVERITY_SCAN_TOKEN 2>/dev/null)

.PHONY: all
.PHONY: clean
.PHONY: tags
.PHONY: test
.PHONY: valgrind
.PHONY: lib

all : libs version.h ename.c.inc $(EXOUTPUT)
libs : $(LIBS_OBJ)
$(EXOUTPUT) : libs version.h

version.h: $(SRC)
	@../scripts/setver.sh

ename.c.inc:
	@bash ../scripts/build_ename.sh > ename.c.inc

build/lib/inih/%.o: ${INIH_DIR}/%.c
	@mkdir -p build/lib/inih
	@echo -e " * $(@F)"
	@$(CC) $(INCLUDES) $(TARGET_CFLAGS) -c $< -o $@

build/lib/natsort/%.o: ${NATSORT_DIR}/%.c
	@mkdir -p build/lib/natsort
	@echo -e " * $(@F)"
	@$(CC) $(INCLUDES) $(TARGET_CFLAGS) -c $< -o $@

build/%.o: %.c
	@mkdir -p build
	@echo -e " * $(@F)"
	@$(CC) $(INCLUDES) $(TARGET_CFLAGS) -c $< -o $@

%.o: %.c
	@echo -e " * $(@F)"
	@$(CC) $(INCLUDES) $(TARGET_CFLAGS) -c $< -o $@

$(EXOUTPUT): $(LOCAL_OBJECTS) $(INIH_OBJ) $(NATSORT_OBJ)
	@echo -e "\n>> Building $@ as target $(TARGET)"
	@$(CC) $(TARGET_LDFLAGS) $(LOCAL_OBJECTS) $(INIH_OBJ) $(NATSORT_OBJ) -o $@ && size $@

clean :
	@${RM} -rf build
	@${RM} -f ${PROJ}
	@${RM} -rf cov-int

clean-lib:
	@${RM} -rf build/lib

tags: $(SRCS)
	@echo "Generating tags .."
	@exuberant-ctags -R

test: $(PROJ)
	@bash ../unit_tests/run_tests.sh

valgrind: $(PROJ)
	@bash ../unit_tests/run_tests.sh --valgrind

cppcheck: $(PROJ)
	@cppcheck --quiet --force --enable=warning --enable=performance --enable=information --enable=performance --enable=portability -I ./ ./

scan-build: clean
	@scan-build make

coverity: clean clean-lib
	@cov-build --dir cov-int make
	@tar czvf /tmp/$(PROJ).tgz cov-int
	@curl --form token=$(COVERITY_SCAN_TOKEN) \
	  --form email=2b4eda@subdimension.ro \
	  --form file=@/tmp/$(PROJ).tgz \
	  --form version="$(shell ../scripts/getver.sh -i ./version.h -t MAJ.MINbBUILD)" \
	  --form description="Description" \
	  https://scan.coverity.com/builds?project=rodan%2Feecu-sat

force_look:
	@true
