// Unit tests for workspace/all/common/utils.c
//
// Only what has a seam worth testing: utils.c links against libc alone - no SDL
// symbols, no project symbols - so it drops straight into a test binary. The menu
// itself is one 4000-line translation unit with everything static, so its logic is
// covered by driving the built app instead (see bench/ and the PR notes).
//
// Plain asserts, no framework. Run with `make test`.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "utils.h"
#include "defines.h"

static int failures = 0;
static const char* section = "";

#define CHECK(cond, msg) do { \
	if (!(cond)) { \
		printf("  FAIL  %s: %s\n         at %s:%d\n", section, msg, __FILE__, __LINE__); \
		failures++; \
	} \
} while (0)

#define SECTION(name) do { section = name; printf("- %s\n", name); } while (0)

static const char* slurp(const char* path) {
	static char buf[512];
	FILE* f = fopen(path, "r");
	if (!f) return NULL;
	size_t n = fread(buf, 1, sizeof(buf)-1, f);
	buf[n] = '\0';
	fclose(f);
	return buf;
}

///////////////////////////////////////

static void test_atomic_write(char* dir) {
	char target[MAX_PATH], tmp[MAX_PATH], expected_tmp[MAX_PATH];
	snprintf(target, sizeof(target), "%s/list.txt", dir);
	snprintf(expected_tmp, sizeof(expected_tmp), "%s/list.txt.tmp", dir);

	SECTION("openAtomic/commitAtomic write a new file");
	FILE* file = openAtomic(target, tmp, sizeof(tmp));
	CHECK(file != NULL, "openAtomic returns a handle");
	if (file) {
		fputs("first\n", file);
		CHECK(commitAtomic(file, target, tmp) == 0, "commitAtomic reports success");
	}
	CHECK(slurp(target) && strcmp(slurp(target), "first\n")==0, "target holds the new contents");
	CHECK(access(expected_tmp, F_OK) != 0, "no temp file is left behind");

	SECTION("a rewrite replaces the file whole, so a shorter one leaves no tail");
	file = openAtomic(target, tmp, sizeof(tmp));
	if (file) {
		fputs("considerably longer than what follows\n", file);
		commitAtomic(file, target, tmp);
	}
	file = openAtomic(target, tmp, sizeof(tmp));
	if (file) {
		fputs("hi\n", file);
		commitAtomic(file, target, tmp);
	}
	CHECK(slurp(target) && strcmp(slurp(target), "hi\n")==0, "none of the longer contents survive");

	// This is the property the whole helper exists for: if the rename cannot happen,
	// what was already on the card must be exactly as it was.
	SECTION("a failed commit leaves the original untouched");
	char blocked[MAX_PATH], inner[MAX_PATH], blocked_tmp[MAX_PATH];
	snprintf(blocked, sizeof(blocked), "%s/blocked", dir);
	snprintf(inner, sizeof(inner), "%s/blocked/child", dir);
	snprintf(blocked_tmp, sizeof(blocked_tmp), "%s/blocked.tmp", dir);
	mkdir(blocked, 0777);
	FILE* child = fopen(inner, "w");
	if (child) { fputs("original", child); fclose(child); }

	// renaming a file over a non-empty directory always fails, root or not
	file = openAtomic(blocked, tmp, sizeof(tmp));
	CHECK(file != NULL, "a temp file opens even when the target cannot be replaced");
	if (file) {
		fputs("should never land\n", file);
		CHECK(commitAtomic(file, blocked, tmp) == -1, "commitAtomic reports the failure");
	}
	struct stat info;
	CHECK(stat(blocked, &info)==0 && S_ISDIR(info.st_mode), "the original target still exists");
	CHECK(slurp(inner) && strcmp(slurp(inner), "original")==0, "its contents are unchanged");
	CHECK(access(blocked_tmp, F_OK) != 0, "the temp file is cleaned up after the failure");

	SECTION("openAtomic refuses a path with no room for the suffix");
	char small[8];
	CHECK(openAtomic("/tmp/a/rather/long/path.txt", small, sizeof(small)) == NULL,
		"returns NULL rather than writing to a truncated path");
	CHECK(commitAtomic(NULL, target, small) == -1, "commitAtomic on a NULL handle is a safe no-op");
	CHECK(slurp(target) && strcmp(slurp(target), "hi\n")==0, "and disturbs nothing else");

	SECTION("openAtomic reports a temp file it cannot create");
	CHECK(openAtomic("/proc/nonexistent/nope.txt", tmp, sizeof(tmp)) == NULL, "returns NULL");
	CHECK(tmp[0]=='\0', "and clears tmp_path so a caller cannot act on a stale one");
}

///////////////////////////////////////

static void test_matching(void) {
	// prefixMatch guards "is this path under Roms", so case must not matter
	SECTION("prefixMatch");
	CHECK(prefixMatch("/Roms", "/Roms/Game Boy (GB)/x.gb"), "matches a prefix");
	CHECK(prefixMatch("/roms", "/ROMS/x.gb"), "ignores case");
	CHECK(!prefixMatch("/Roms", "/Rom"), "a truncated candidate does not match");
	CHECK(prefixMatch("", "anything"), "an empty prefix matches everything");

	// suffixMatch decides whether a multi-disc folder is keyed on its m3u or cue,
	// which is what stops the same game being favorited twice under two names
	SECTION("suffixMatch");
	CHECK(suffixMatch(".gb", "Kirby's Dream Land.gb"), "matches an extension");
	CHECK(suffixMatch(".GB", "kirby.gb"), "ignores case");
	CHECK(suffixMatch(".m3u", "Final Fantasy VII/Final Fantasy VII.m3u"), "matches through a path");
	CHECK(!suffixMatch(".cue", "Final Fantasy VII.m3u"), "does not match a different extension");
	CHECK(!suffixMatch(".gb", "gb"), "a suffix longer than the string does not match");

	// exactMatch is the odd one out: it is case SENSITIVE, unlike its neighbours.
	// Asserted as observed rather than as intended - hide() inherits it below.
	SECTION("exactMatch");
	CHECK(exactMatch("map.txt", "map.txt"), "matches identical strings");
	CHECK(!exactMatch("map.txt", "MAP.TXT"), "is case sensitive, unlike prefix/suffix match");
	CHECK(!exactMatch("ab", "abc"), "a prefix is not a match");
	CHECK(!exactMatch(NULL, "a"), "NULL on the left is not a match");
	CHECK(!exactMatch("a", NULL), "NULL on the right is not a match");

	// containsString is the entire rom search filter
	SECTION("containsString");
	CHECK(containsString("Legend of Zelda, The - Oracle of Ages", "zel"), "finds a substring, ignoring case");
	CHECK(containsString("Legend of Zelda", "ZELDA"), "ignores case in the needle too");
	CHECK(containsString("Legend of Zelda", "Legend"), "matches at the start");
	CHECK(!containsString("Legend of Zelda", "Metroid"), "reports a miss");
	CHECK(containsString("anything", ""), "an empty needle matches");

	SECTION("hide");
	CHECK(hide(".hidden"), "dotfiles are hidden");
	CHECK(hide("core.disabled"), "disabled files are hidden");
	CHECK(hide("map.txt"), "map.txt is hidden");
	CHECK(!hide("Kirby's Dream Land.gb"), "an ordinary rom is not hidden");
}

static void test_names(void) {
	char out[MAX_PATH];

	SECTION("getDisplayName");
	getDisplayName("/Roms/Game Boy (GB)/Kirby's Dream Land (USA, Europe).gb", out);
	CHECK(strcmp(out, "Kirby's Dream Land")==0, "drops directories, extension and region parens");
	getDisplayName("Metroid II - Return of Samus.gb", out);
	CHECK(strcmp(out, "Metroid II - Return of Samus")==0, "keeps a name that has nothing to strip");
	getDisplayName("(USA).gb", out);
	CHECK(out[0] != '\0', "never reduces a name to nothing");

	SECTION("trimSortingMeta");
	char buf[64];
	strcpy(buf, "001) Sonic the Hedgehog");
	char* p = buf;
	trimSortingMeta(&p);
	CHECK(strcmp(p, "Sonic the Hedgehog")==0, "strips a sorting prefix");
	strcpy(buf, "Sonic the Hedgehog");
	p = buf;
	trimSortingMeta(&p);
	CHECK(strcmp(p, "Sonic the Hedgehog")==0, "leaves a name without one alone");
}

///////////////////////////////////////

int main(void) {
	char dir[] = "/tmp/nextui-tests-XXXXXX";
	if (!mkdtemp(dir)) {
		perror("mkdtemp");
		return 1;
	}

	printf("running utils tests\n");
	test_atomic_write(dir);
	test_matching();
	test_names();

	if (failures) printf("\n%d check(s) FAILED\n", failures);
	else printf("\nall checks passed\n");
	return failures ? 1 : 0;
}
