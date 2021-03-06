#include <stdio.h>
#include <pdict.h>
#include <string.h>

#define readstr(name, buff)         \
	setbuf(stdin, NULL);            \
	printf("%s: ", name);           \
	fscanf(stdin, " %[^\n]", buff); \
	setbuf(stdin, NULL);

int main(int argc, char const *argv[]) {
	int32_t max_len = __PDICT_DEFAULT_MAX_LEN__;
	bool add_missing = true;
	int32_t stretch = 0;
	if (argc > 1) {
		max_len = atoi(argv[1]);

		if (argc > 2) {
			add_missing = atoi(argv[2]);

			if (argc > 3) {
				stretch = atoi(argv[3]);
			}
		}
	}
	pdict_t *dict = pdict_create_all(max_len, add_missing, stretch);

	int op = 0;
	char str1[2048];
	char str2[2048];
	do {
		printf(
			" 0: exit\n"
			" 1: set value\n"
			" 2: get value\n"
			" 3: has key\n"
			" 4: get keys\n"
			" 5: get keys sorted\n"
			"\nop: "
		);

		setbuf(stdin, NULL);
		scanf(" %d", &op);

		if (op == 1) {
			readstr("key", str1);
			readstr("value", str2);

			char *value = strdup(str2);

			pdict_set_value(dict, str1, value);
		} else if (op == 2) {
			readstr("key", str1);

			const char *value = pdict_get_value(dict, str1);

			printf("value: %s\n", value);
		} else if (op == 3) {
			readstr("key", str1);

			printf("%s\n", pdict_has_key(dict, str1) ? "true": "false");
		} else if (op == 4) {
			int32_t len;
			char **ks = pdict_get_keys(dict, &len, false);

			for (int i = 0; i < len; i++) {
				printf("%s ", ks[i]);
				free(ks[i]);
			}
			puts("");
			free(ks);
		} else if (op == 5) {
			int32_t len;
			char **ks = pdict_get_keys(dict, &len, true);

			for (int i = 0; i < len; i++) {
				printf("%s ", ks[i]);
				free(ks[i]);
			}
			puts("");
			free(ks);
		}
	} while(op != 0);

	pdict_destroy(dict);
	return 0;
}
