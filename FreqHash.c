/* 
 * FreqHash.c
 * 
 * A hash table specially designed for counting letter frequency.
 * 
 * In order to use this hash table you must include stdio, stdlib and string.
 */

#define DEFAULT_CAPACITY 10
#define RESIZE_MIN 16

typedef struct {
	char *key;
	double value;
} Pair;

typedef struct {
	Pair *pairs;
	size_t length; /* length of pairs array */
} Bucket;

typedef struct {
	Bucket *buckets;
	size_t length; /* length of buckets array */
	size_t count; /* number of items in buckets array */
} Hash;


/*
 * Allocates (hash) with the default capacity.
 */
int hash_init(Hash *hash);

/*
 * Allocates (hash) with the given capacity.
 */
int hash_init_capacity(Hash *hash, size_t capacity);

/* 
 * Clears (hash), deleting its contents and clearing all allocated memory.
 */
int hash_clear(Hash *hash);

/* 
 * Determines whether (key) exists in (hash). If so, returns nonzero; if not, returns 
 * zero.
 */
int hash_exists(Hash hash, char *key);

/* 
 * Finds (key) in (hash) and returns its corresponding value. A result of -1 indicates 
 * that the key was not found.
 */
long hash_get(Hash hash, char *key);

/* 
 * This is a function that is particularly useful when counting letter frequency. It 
 * finds (key) in (hash) and increases it by (value). If (key) is not found, it is created 
 * and its value is set to (value).
 */
int hash_inc(Hash *hash, const char *key, double value);

/* 
 * Prints (hash) to the output stream.
 */
int hash_print(Hash hash);

/* 
 * Puts (key) and (value) as a pair into (hash).
 */
long hash_put(Hash *hash, const char *key, double value);

/* Takes a function and calls that function for each key-value pair in Hash. 
 * If f returns a nonzero value, this function exits and returns that value.
 */
int hash_foreach(Hash hash, int (*f)(const char *key, double value));

/* 
 * Sorts (hash) by value and puts the resulting array in (res).
 * 
 * res: A pointer to an array, for the sorted hash to be placed in. This function will 
 *   allocate res, so do not pass in an already-allocated pointer.
 * length: The length of res. The function will set this value.
 */
int hash_sort(Pair **res, size_t *length, Hash hash);
int pair_comparator(const void *x, const void *y);

int hash_test();

size_t hash_function(const char *key);
void * hash_malloc(size_t size);
void * hash_realloc(void *ptr, size_t size);
int hash_resize(Hash *hash);
size_t next_power_of_2(size_t x);
size_t next_size(size_t x);
int do_resize(size_t x);


int hash_init(Hash *hash)
{
	return hash_init_capacity(hash, DEFAULT_CAPACITY);
}

int hash_init_capacity(Hash *hash, size_t capacity)
{
	hash->buckets = hash_malloc(sizeof(Bucket) * next_size(capacity));
	hash->length = next_size(capacity);
	hash->count = 0;
	return 0;
}

int hash_clear(Hash *hash)
{
	size_t i, j;
	for (i = 0; i < hash->length; ++i)
		if (hash->buckets[i].pairs) {
			for (j = 0; j < hash->buckets[i].length; ++j)
				free(hash->buckets[i].pairs[j].key);
			free(hash->buckets[i].pairs);
		}
	
	free(hash->buckets);
	hash->length = 0;
	return 0;
}

int hash_exists(Hash hash, char *key)
{	
	size_t j, i = hash_function(key) % hash.length;
	
	if (hash.buckets[i].pairs) {		
		// Find the key in the bucket.
		for (j = 0; j < hash.buckets[i].length; ++j)
			if (strcmp(hash.buckets[i].pairs[j].key, key) == 0)
				return 1;
	}
	
	// The key does not exist in the bucket.
	return 0;	
}

long hash_get(Hash hash, char *key)
{
	size_t j, i = hash_function(key) % hash.length;
	
	if (hash.buckets[i].pairs) {		
		// Find the key in the bucket.
		for (j = 0; j < hash.buckets[i].length; ++j)
			if (strcmp(hash.buckets[i].pairs[j].key, key) == 0)
				return hash.buckets[i].pairs[j].value;
	}
	
	// The key does not exist in the bucket.
	return -1;
}

int hash_inc(Hash *hash, const char *key, double value)
{
	size_t j, i = hash_function(key) % hash->length;
		
	if (hash->buckets[i].pairs) {		
		// Find the pair in the bucket.
		for (j = 0; j < hash->buckets[i].length; ++j)
			if (strcmp(hash->buckets[i].pairs[j].key, key) == 0) {
				hash->buckets[i].pairs[j].value += value;
				return 0;
			}
		
		// The pair does not exist in the bucket.
		
		// Resize the bucket if necessary.
		if (resize_p(hash->buckets[i].length)) {
			hash->buckets[i].pairs = hash_realloc(hash->buckets[i].pairs, 
					sizeof(Pair) * next_size(hash->buckets[i].length + 1));
			for (j = hash->buckets[i].length; j < next_size(hash->buckets[i].length + 1); ++j)
				hash->buckets[i].pairs[j].key = NULL;
		}
		
		// Put the new pair at the end of the bucket.
		hash->buckets[i].pairs[j].key = hash_malloc(strlen(key) + 1);
		strcpy(hash->buckets[i].pairs[j].key, key);
		hash->buckets[i].pairs[j].value = value;
		++hash->buckets[i].length;
		
	} else {
		// The bucket does not exist. Create a new bucket and put the pair in it.
		hash->buckets[i].pairs = hash_malloc(sizeof(Pair) * next_size(1));
		hash->buckets[i].length = 1;
		hash->buckets[i].pairs[0].key = hash_malloc(strlen(key) + 1);
		strcpy(hash->buckets[i].pairs[0].key, key);
		hash->buckets[i].pairs[0].value = value;
	}
	
	++hash->count;
	if (hash->count * 100 > hash->length * 75) {
		hash_resize(hash);
	}
		
	return 0;
}

int hash_merge(Hash *dest, Hash src)
{
	size_t i, j;
	for (i = 0; i < src.length; ++i)
		for (j = 0; j < src.buckets[i].length; ++j)
			hash_inc(dest, src.buckets[i].pairs[j].key, src.buckets[i].pairs[j].value);
	return 0;
}

int hash_print(Hash hash)
{
	size_t i, j;
	for (i = 0; i < hash.length; ++i)
		for (j = 0; j < hash.buckets[i].length; ++j)
			if (hash.buckets[i].pairs[j].key)
				printf("%s => %.8f, ", hash.buckets[i].pairs[j].key, hash.buckets[i].pairs[j].value);
	
	printf("\n");
	return 0;
}

long hash_put(Hash *hash, const char *key, double value)
{
	size_t j, i = hash_function(key) % hash->length;
	
	if (hash->buckets[i].pairs) {		
		// Find the pair in the bucket.
		for (j = 0; j < hash->buckets[i].length; ++j)
			if (strcmp(hash->buckets[i].pairs[j].key, key) == 0) {
				hash->buckets[i].pairs[j].value = value;
				return 0;
			}
		
		// The pair does not exist in the bucket.
		
		// Resize the bucket if necessary.
		if (resize_p(hash->buckets[i].length)) {
			hash->buckets[i].pairs = hash_realloc(hash->buckets[i].pairs, 
					sizeof(Pair) * next_size(hash->buckets[i].length + 1));
			for (j = hash->buckets[i].length; j < next_size(hash->buckets[i].length + 1); ++j)
				hash->buckets[i].pairs[j].key = NULL;
		}
		
		// Put the new pair at the end of the bucket.
		hash->buckets[i].pairs[j].key = hash_malloc(strlen(key) + 1);
		strcpy(hash->buckets[i].pairs[j].key, key);
		hash->buckets[i].pairs[j].value = value;
		++hash->buckets[i].length;
		
	} else {
		// The bucket does not exist. Create a new bucket and put the pair in it.
		hash->buckets[i].pairs = hash_malloc(sizeof(Pair) * next_size(1));
		hash->buckets[i].length = 1;
		hash->buckets[i].pairs[0].key = hash_malloc(strlen(key) + 1);
		strcpy(hash->buckets[i].pairs[0].key, key);
		hash->buckets[i].pairs[0].value = value;
	}
	
	++hash->count;
	if (hash->count * 100 > hash->length * 75) {
		hash_resize(hash);
	}
	
	
	return 0;
}

int hash_foreach(Hash hash, int (*f)(const char *key, double value))
{
	int i, j, ret = 0;
	
	for (i = 0; i < hash.length; ++i)
		for (j = 0; j < hash.buckets[i].length; ++j)
			if (hash.buckets[i].pairs[j].key) {
				ret = (*f)(hash.buckets[i].pairs[j].key, 
						hash.buckets[i].pairs[j].value);
				if (ret != 0)
					return ret;
			}	
	
	return 0;
}

int hash_sort(Pair **res, size_t *length, Hash hash)
{
	*length = hash.count;
	*res = malloc(sizeof(Bucket) * hash.count);
	
	size_t i, j, k = 0;
	for (i = 0; i < hash.length; ++i) {
		for (j = 0; j < hash.buckets[i].length; ++j) {
			(*res)[k] = hash.buckets[i].pairs[j];
			++k;
		}
	}
	
	qsort(*res, k, sizeof(Pair), &pair_comparator);
	*length = k;
	return 0;
}

int pair_comparator(const void *x, const void *y)
{
	const Pair *xp = (const Pair *) x;
	const Pair *yp = (const Pair *) y;
	
	if (xp->value > yp->value) return -1;
	else if (xp->value < yp->value) return 1;
	else return 0;
}

int hash_test()
{
	Hash hash;
	hash_init(&hash);
	hash_put(&hash, "hello", 1);
	hash_put(&hash, "hello", 3);
	hash_inc(&hash, "world", 5);
	hash_inc(&hash, "world", 5);
	hash_print(hash);
	hash_clear(&hash);
	return 0;
}

size_t hash_function(const char *key)
{
	size_t x = 5381;
	--key;
	while (*(++key))
		x = 33*x + *key;
	
	return x;
}

void * hash_malloc(size_t size)
{
	void *mem = malloc(size);
	if (mem == NULL)
		fprintf(stderr, "Error: Memory allocation failed.\n");
	else memset(mem, 0, size);
	
	return mem;
}

void * hash_realloc(void *ptr, size_t size)
{
	void *mem = realloc(ptr, size);
	if (mem == NULL)
		fprintf(stderr, "Error: Memory reallocation failed.\n");
	
	return mem;
}

int hash_resize(Hash *hash)
{
	Hash res;
	hash_init_capacity(&res, sizeof(Bucket) * next_size(hash->length));
	size_t i, j;
	for (i = 0; i < hash->length; ++i) {
		for (j = 0; j < hash->buckets[i].length; ++j)
			if (hash->buckets[i].pairs[j].key)
				hash_put(&res, hash->buckets[i].pairs[j].key, hash->buckets[i].pairs[j].value);
	}
	
	hash_clear(hash);
	*hash = res;
	return 0;		
}


/* 
 * Returns the next power of 2. In the case in which x is already a power of 2, 
 * it will return x << 1. 0 returns 0.
 * 
 * Source: http://graphics.stanford.edu/~seander/bithacks.html
 */
size_t next_power_of_2(size_t x)
{
	// This ORs x with every possible 1 bit up to its length, so every bit will 
	// be filled. Then it adds 1 which for instance turns (e.g.) 111 into 1000.
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	return x + 1;
}

/* 
 * next_size() and resize_p() are used to determine whether an array of a given size 
 * needs to be resized. The length of the array is always a power of 2. When the array 
 * needs to be resized, it is reallocated to be twice as large.
 */
size_t next_size(size_t x)
{
	return x < RESIZE_MIN ? RESIZE_MIN : next_power_of_2(x);
}

int resize_p(size_t x)
{
	// Iff x is a power of 2, then x and (x-1) will share no digits. For instance, 
	// 1000 & 111 = 0. Do not resize unless x is greater than a certain minimum.
	return x >= RESIZE_MIN && !(x & (x - 1));	
}