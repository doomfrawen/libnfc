/*-
 * Regression checks for the Windows setenv()/unsetenv() replacements.
 * Licensed under the GNU Lesser General Public License, version 3 or later.
 *
 * Run on Windows without NFC hardware or libusb:
 * gcc -std=c99 -Wall -Wextra -Werror -I. test/test_win32_stdlib.c \
 *   contrib/win32/stdlib.c -o test_win32_stdlib.exe
 * ./test_win32_stdlib.exe
 */

#ifndef _WIN32
#error These tests require the Windows CRT.
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int setenv(const char *name, const char *value, int overwrite);
void unsetenv(const char *name);

static int failures;
static int checks;

#define CHECK(condition) do { \
  checks++; \
  if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
    failures++; \
  } \
} while (0)

static int
value_is(const char *name, const char *expected)
{
  const char *value = getenv(name);
  return value && strcmp(value, expected) == 0;
}

static void
test_overwrite(void)
{
  const char *name = "NFC_ENV_TEST";
  unsetenv(name);
  CHECK(setenv(name, "first", 0) == 0);
  CHECK(value_is(name, "first"));
  CHECK(setenv(name, "second", 0) == 0);
  CHECK(value_is(name, "first"));
  CHECK(setenv(name, "second", 1) == 0);
  CHECK(value_is(name, "second"));
  CHECK(setenv(name, "third", -1) == 0);
  CHECK(value_is(name, "third"));
  unsetenv(name);
  CHECK(getenv(name) == NULL);
  unsetenv(name);
  CHECK(getenv(name) == NULL);
}

static void
test_long_strings(void)
{
  char name[81];
  char value[1025];
  memset(name, 'N', sizeof(name) - 1);
  name[sizeof(name) - 1] = '\0';
  memset(value, 'v', sizeof(value) - 1);
  value[sizeof(value) - 1] = '\0';

  CHECK(setenv(name, "short", 1) == 0);
  CHECK(value_is(name, "short"));
  unsetenv(name);
  CHECK(getenv(name) == NULL);

  CHECK(setenv("NFC_ENV_TEST", value, 1) == 0);
  CHECK(value_is("NFC_ENV_TEST", value));
  unsetenv("NFC_ENV_TEST");
  CHECK(getenv("NFC_ENV_TEST") == NULL);

  CHECK(setenv(name, value, 1) == 0);
  CHECK(value_is(name, value));
  CHECK(setenv(name, "ignored", 0) == 0);
  CHECK(value_is(name, value));
  unsetenv(name);
  CHECK(getenv(name) == NULL);
}

static void
test_argument_copy(void)
{
  char name[] = "NFC_ENV_COPY";
  char value[] = "left=right";
  CHECK(setenv(name, value, 1) == 0);
  memset(name, 'x', sizeof(name) - 1);
  memset(value, 'y', sizeof(value) - 1);
  CHECK(value_is("NFC_ENV_COPY", "left=right"));
  unsetenv("NFC_ENV_COPY");
  CHECK(getenv("NFC_ENV_COPY") == NULL);
}

static void
test_invalid_arguments(void)
{
  CHECK(setenv("NFC_ENV_TEST", "keep", 1) == 0);
  CHECK(setenv(NULL, "value", 1) == -1);
  CHECK(setenv("", "value", 1) == -1);
  CHECK(setenv("NFC_ENV_TEST=bad", "value", 1) == -1);
  CHECK(setenv("NFC_ENV_TEST", NULL, 1) == -1);
  CHECK(setenv("NFC_ENV_TEST", NULL, 0) == -1);
  unsetenv(NULL);
  unsetenv("");
  unsetenv("NFC_ENV_TEST=bad");
  CHECK(value_is("NFC_ENV_TEST", "keep"));
  unsetenv("NFC_ENV_TEST");
  CHECK(getenv("NFC_ENV_TEST") == NULL);
}

static void
test_windows_empty_value(void)
{
  // Preserve the Windows CRT convention: an empty value removes the entry.
  CHECK(setenv("NFC_ENV_TEST", "keep", 1) == 0);
  CHECK(setenv("NFC_ENV_TEST", "", 0) == 0);
  CHECK(value_is("NFC_ENV_TEST", "keep"));
  CHECK(setenv("NFC_ENV_TEST", "", 1) == 0);
  CHECK(getenv("NFC_ENV_TEST") == NULL);
}

int
main(int argc, char **argv)
{
  test_overwrite();
  // This mode also runs safely against the previous 32-byte implementation.
  if (!(argc == 2 && strcmp(argv[1], "--overwrite-only") == 0)) {
    test_long_strings();
    test_argument_copy();
    test_invalid_arguments();
    test_windows_empty_value();
  }
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
