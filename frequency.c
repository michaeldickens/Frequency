/* 
 * A C program designed to calculate frequency for given files. Main revision 
 * created 2011-12-26.
 */

#include <ctype.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreqHash.c"

#define MAX_WORD_LEN 1000

#define ASCII_SHIFT 14

#define CASE_SENSITIVE_P true
#define CTRL_TO_ESCAPE_P true

#define MAX_TOKENS_TO_PRINT 0

/* 
 * Reads a number of files and calculates the aggregate frequency.
 */
int freq_read_files(Hash *hash, const char *regex);

/* 
 * Reads a file and counts the frequency of each regex match.
 * hash: A hash in which to put the resulting matches, where each match is paired with the 
 *   number of times it occurs.
 * filename: The name of the file to be read.
 * regex: Each character sequence placed in hash must match this regular expression.
 * multiplier: A single instance of a sequence is counted as this many instances.
 * 
 * Return Codes
 * -0: Success.
 * -1: File read error.
 * -2: Invalid regular expression.
 * -3: File contains a sequence that exceeds the maximum length.
 * -4: Regular expression ran out of memory.
 * 
 */ 
int freq_read_file(Hash *hash, const char *filename, const char *regex, int multiplier);

/* 
 * Find all sequences of (wordcount) words. This does not work as a regex, so 
 * it has its own function.
 */
int find_n_words(Hash *hash, int wordcount);
int find_n_words_for_file(Hash *hash, const char *filename, int wordcount, int multiplier);

/* Increase the value of (sequence) in the hash function (hash).
 */
int freq_hash_inc(Hash *hash, char *sequence, double value, regmatch_t matchptr[]);

/* Apply a filter to every char in (buffer).
int filter_chars(char *buffer);

/* Scan a buffer and add regex matches to the hash.
 */
int freq_scan(Hash *hash, char *buffer, uint64_t length, regex_t compiled, 
		bool overlap, double adjusted_multiplier);

bool legal_chars(const char *sequence, size_t length);

int print_sequence(FILE *stream, const char *sequence, bool ctrl_to_escape);

int read_file(char **buffer, uint64_t *length, const char *filename);

/* 
 * REGEX CREATION TIPS
 *
 * If the regex contains a subexpression, this program will treat the first 
 * subexpression as the target text. You can use this to e.g. match the 
 * first letter in a word. The order of subexpressions is the order in which 
 * they begin.
 */

#define FREQ_LETTER_CHARS "[a-z]"
#define FREQ_LETTER_DIGRAPHS "[a-z]{2,2}"
#define FREQ_LETTER_TRIGRAPHS "[a-z]{3,3}"
#define FREQ_MAIN30_CHARS "[a-z.,;']"
#define FREQ_MAIN30_DIGRAPHS "[a-z.,;']{2,2}"
#define FREQ_MAIN30_TRIGRAPHS "[a-z.,;']{3,3}"
#define FREQ_DIGRAPHS_NOSPC "[^\n\t ]{2,2}"
#define FREQ_CHARS "."
#define FREQ_DIGRAPHS ".."
#define FREQ_TRIGRAPHS "..."

// a word cannot have ' at beginning or end
#define FREQ_WORDS "((([a-z])+('[a-z])?)+)"

// BUG: Does not work for unknown reason.
#define FREQ_NUMBERS "((\\+|-)?[0-9]+(\\.[0-9]+)?((e|E)[0-9]+)?)"

#define FREQ_FIRST_LETTER "([a-z])[a-z]*"
// BUG: these do not work because when the string fails to match, it is deleted
#define FREQ_SECOND_LETTER "[a-z]([a-z])[a-z]*"
#define FREQ_THIRD_LETTER "[a-z]{2,2}([a-z])[a-z]*"
#define FREQ_LAST_LETTER "[a-z]*([a-z])"
#define FREQ_FIRST_DIGRAPH "([a-z]{2,2})[a-z]*"
#define FREQ_LAST_DIGRAPH "[a-z]*([a-z]{2,2})"

static const char *files[] = {
	"000bigfiles/00allProse.txt", 
	"000bigfiles/01allCasual.txt", 
	"000bigfiles/02allC.txt", 
	"000bigfiles/02allJava.txt", 
	"000bigfiles/02allPerl.txt", 
	"000bigfiles/02allRuby.txt", 
	"000bigfiles/03allFormal.txt", 
	"000bigfiles/04allNews.txt", 
};

static const char *files_no_prog[] = {
	"000bigfiles/00allProse.txt", 
	"000bigfiles/01allCasual.txt", 
	"000bigfiles/03allFormal.txt", 
	"000bigfiles/04allNews.txt", 	
};

static int multipliers[] = {
	18, 25, 4, 2, 1, 1, 15, 20
};

static int muls_no_prog[] = {
	18, 25, 15, 20
};

int main(int argc, char const *argv[])
{
	Hash hash;
	hash_init(&hash);
	
	// tests
//	freq_read_file(&hash, "000bigfiles/test.txt", FREQ_MAIN30_CHARS, 1); // works
//	freq_read_file(&hash, "000bigfiles/test.txt", FREQ_DIGRAPHS, 1); // works
//	freq_read_file(&hash, "000bigfiles/test.txt", FREQ_NUMBERS, 1); // FAILS
//	freq_read_file(&hash, "000bigfiles/02allC.txt", FREQ_CHARS, 1);
//	find_n_words(&hash, 3);
	
	find_n_words_for_file(&hash, "000bigfiles/0 prose/0 shakespeare DO NOT USE.txt", 2, 1);
	
	Pair *pairs;
	size_t length;
	hash_sort(&pairs, &length, hash);
	
	if (MAX_TOKENS_TO_PRINT > 0 && length > MAX_TOKENS_TO_PRINT)
		length = MAX_TOKENS_TO_PRINT;
	
	print_pairs(pairs, length);
	
	hash_clear(&hash);
	free(pairs);
		
	return 0;
}

int print_pairs(Pair *pairs, size_t length)
{
	size_t i;
	for (i = 0; i < length; ++i) {
		print_sequence(stdout, pairs[i].key, CTRL_TO_ESCAPE_P);
		printf(" %lld\n", (long long) (pairs[i].value));
	}
	
	printf("\n");
	return 0;	
}

int print_pairs_short(Pair *pairs, size_t length)
{
	size_t i;
	for (i = 0; i < length; ++i) {
		print_sequence(stdout, pairs[i].key, true);
		printf(" ");
	}
	
	printf("\n");
	return 0;	
}

int freq_read_files_programming(Hash *hash, const char *regex)
{
	const char *test_files[] = {
		"000bigfiles/02allC.txt", 
		"000bigfiles/02allJava.txt", 
		"000bigfiles/02allPerl.txt", 
		"000bigfiles/02allRuby.txt", 
	};
	
	int test_muls[] = { 4, 2, 1, 1 };
	
	int ret = 0;
	size_t i;
	for (i = 0; i < sizeof(test_files)/sizeof(const char *); ++i) {
		ret = freq_read_file(hash, test_files[i], regex, test_muls[i]);
		if (ret) return ret;
		printf("done with %s at %d\n", test_files[i], test_muls[i]);
	}
	
	return ret;	
}

int freq_read_files_test(Hash *hash, const char *regex)
{
	const char *files[] = {
		"000bigfiles/test.txt", 
		"000bigfiles/1 net 1.txt"
	};
	
	int multipliers[] = { 4, 5 };
	
	int ret = 0;
	size_t i;
	for (i = 0; i < sizeof(files)/sizeof(const char *); ++i) {
		ret = freq_read_file(hash, files[i], regex, multipliers[i]);
		if (ret) return ret;
		printf("done with %s at %d\n", files[i], multipliers[i]);
	}
	
	return ret;
}

/* 
 * Reads the global array `files` and calls `freq_read_file()` on each
 * file in the array.
 */
int freq_read_files(Hash *hash, const char *regex)
{
	int ret = 0;
	size_t i;
	for (i = 0; i < sizeof(files)/sizeof(const char *); ++i) {
		ret = freq_read_file(hash, files[i], regex, multipliers[i]);
		if (ret) return ret;
		printf("done with %s at %d\n", files[i], multipliers[i]);
	}
	
	return ret;
}

int filter_chars(char *buffer)
{
	size_t i, length = strlen(buffer);
	for (i = 0; i < length; ++i) {
		buffer[i] = (char) tolower((int) buffer[i]);
	}
	
	return 0;
}

/*
 * Reads the file at `filename`. Finds all matches for the given
 * regular expression and counts their frequency, storing the result
 * in `hash`. The frequencies are multiplied by `multiplier`. Use this
 * if you want to read multiple files and weight some more heavily
 * than others.
 */
int freq_read_file(Hash *hash, const char *filename, const char *regex, int multiplier)
{
	int matches = 0;
	regmatch_t matchptr[2], prev_matchptr[2];
	
	/* For fixed-length sequences, look for overlaps. For variable-length 
	 * sequences, do not.
	 */

	bool overlap = true;
	if (strchr(regex, '+') || strchr(regex, '*'))
		overlap = false;
	
	regex_t compiled;
	int ret = regcomp(&compiled, regex, REG_EXTENDED | REG_ICASE);
	if (ret) return -2;
	 
	char *buffer = NULL;
	uint64_t length = 0;
	ret = read_file(&buffer, &length, filename);
	if (ret) {
		regfree(&compiled);
		return ret;
	}
	
	filter_chars(buffer);

	/* Count the number of regex matches in the file. */
	int count = freq_scan(NULL, buffer, length, compiled, overlap, 1);
	double adjusted_multiplier = (double) multiplier / count;
	
	freq_scan(hash, buffer, length, compiled, overlap, adjusted_multiplier);
	
	free(buffer);
	regfree(&compiled);
			
	return matches;
}

/* 
 * Finds all n-grams of (wordcount) words. Uses all files except for 
 * programming files.
 */
int find_n_words(Hash *hash, int wordcount)
{
	int ret = 0;
	size_t i;
	for (i = 0; i < sizeof(files_no_prog)/sizeof(const char *); ++i) {
		ret = find_n_words_for_file(hash, files_no_prog[i], wordcount, 
				muls_no_prog[i]);
		if (ret) return ret;
		printf("done with %s at %d\n", files_no_prog[i], muls_no_prog[i]);
	}
	
	return ret;	
}

int find_n_words_for_file(Hash *hash, const char *filename, int wordcount, int multiplier)
{
	char *buffer = NULL;
	uint64_t i = 0, next_i = 0, length = 0;
	int ret = 0;
	
	ret = read_file(&buffer, &length, filename);
	if (ret)
		return ret;
	
	filter_chars(buffer);
	char words[(MAX_WORD_LEN + 1) * wordcount];
	size_t j, k;
	
	while (i < length) {
		i = next_i;			
		j = 0;
		
		for (k = 0; i < length && k < wordcount; ++k) {
			while (i < length && !isalnum(buffer[i]))
				++i;
			
			if (k > 0) words[j++] = ' ';
			
			/* Match a word. */
			if (i < length && isalnum(buffer[i])) {
				while (i < length && 
						(isalnum(buffer[i]) || buffer[i] == '\'')) {
					words[j++] = buffer[i++];					
				}
				if (buffer[i-1] == '\'') {
					--i; --j;
				}
			}
				
			if (k == 0) next_i = i;
		}
		
		/* Only add this set of words to the hash if the above loop did not 
		 * reach EOF before completion.
		 */
		if (k == wordcount) {
			words[j] = '\0';
			hash_inc(hash, words, multiplier);
		} else {
			break;
		}
	}
	
	return 0;
}

int freq_scan(Hash *hash, char *buffer, uint64_t length, regex_t compiled, bool overlap, double adjusted_multiplier)
{
	int ret = 0;
	regmatch_t matchptr[2];
	
	int matches = 0;
	uint64_t i = 0;
	
	while (i < length) {
		matchptr[0].rm_so = matchptr[0].rm_eo = 0;	
		matchptr[1].rm_so = matchptr[1].rm_eo = 0;	

		/* Limit the string size to MAX_WORD_LEN so that the regular expression 
		 * only tries to match the first MAX_WORD_LEN characters, instead of 
		 * the entire file.
		 */
		char holder = buffer[i + MAX_WORD_LEN];
		buffer[i + MAX_WORD_LEN] = '\0';
				
		if ((ret = regexec(&compiled, buffer + i, 2, matchptr, 0)) == 0) {
			if (hash) {
				freq_hash_inc(hash, buffer + i, adjusted_multiplier, matchptr);
			}
			++matches;
		} else if (ret == REG_ESPACE) {
			return -4;
		} else break; /* There are no more matches. */
		
		buffer[i + MAX_WORD_LEN] = holder;

		if (overlap) i += matchptr[0].rm_so + 1;
		else i += matchptr[0].rm_eo;
	}	
	
	if (hash == NULL) return matches;
	else return 0;
}

int freq_hash_inc(Hash *hash, char *sequence, double value, regmatch_t matchptr[])
{
	size_t length;
	
	/* If the regex contained at least one subexpression, use the sequence 
	 * contained within the first subexpression. Otherwise, uses the 
	 * complete sequence.
	 */
	if (matchptr[1].rm_so != matchptr[1].rm_eo) {
		sequence += matchptr[1].rm_so;
		length = matchptr[1].rm_eo - matchptr[1].rm_so;
	} else {
		sequence += matchptr[0].rm_so;
		length = matchptr[0].rm_eo - matchptr[0].rm_so;
	}

	/* Do not add the sequence if it contains any illegal characters. */
	if (!legal_chars(sequence, length)) return 0;
	
	char saved = sequence[length];
	sequence[length] = '\0';
	int ret = hash_inc(hash, sequence, value);
	sequence[length] = saved;
	return ret;
}

bool legal_chars(const char *sequence, size_t length)
{
	size_t i;
	char c;
	for (i = 0; i < length; ++i) {
		c = sequence[i];
		if (!isprint(c) && c != '\n' && c != '\t')
			return false;
		++sequence;
	}
	
	return true;
}

int print_sequence(FILE *stream, const char *sequence, bool ctrl_to_escape)
{
	char c;
	for (c = *sequence; (c = *sequence) != '\0'; ++sequence) {
		if (ctrl_to_escape) {
			if (c == '\n') fprintf(stream, "\\n");
			else if (c == '\t') fprintf(stream, "\\t");
			else if (c == ASCII_SHIFT) fprintf(stream, "\\s");
			else if (c == '\b') fprintf(stream, "\\b");
			else if (c == '\\') fprintf(stream, "\\\\");
			else fprintf(stream, "%c", c);
		} else {
			if (c == '\n') fprintf(stream, "\\n");
			else if (c == ASCII_SHIFT) fprintf(stream, "\\s");
			else if (c == '\b') fprintf(stream, "\\b");
			else fprintf(stream, "%c", c);			
		}
	}
	
	return 0;
}

/* 
 * This may be inefficient on 32-bit machines, but if length is declared as size_t then 
 * this function cannot be guaranteed to work for ASCII files larger than 4 GB.
 */
int read_file(char **buffer, uint64_t *length, const char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) return -1;
	
	uint64_t i, tmp_length = 256;
	*buffer = malloc(sizeof(char) * tmp_length);
	char c;
	for (i = 0; (c = fgetc(fp)) != EOF; ++i) {
		if (i == tmp_length) {
			tmp_length *= 4;
			*buffer = realloc(*buffer, sizeof(char) * tmp_length);
		}
		
		if (!CASE_SENSITIVE_P) c = tolower(c);

		(*buffer)[i] = c;
	}
		
	fclose(fp);
	(*buffer)[i] = '\0';
	*length = i;
	return 0;
}
