
// #=====================================================#
// #                                                     #
// # stdfuncs.h ==> implementations of standard funcs    #
// #                (these are mostly terrible)          #
// #=====================================================#

// TODO: make these smaller in the executable

#pragma once

#include <stdarg.h>
#include <intrin.h>
#include <Windows.h>
#pragma intrinsic(__movsb, __stosb)

#define strcat _strcat
#define strcat_s _strcat_s
#define strcpy _strcpy
#define strcpy_s _strcpy_s
#define strcmp _strcmp
#define strlen _strlen

#define tolower __tolower
#define itoa __itoa
#define ultoa __ultoa

// these need to be fast
#define memcpy fast_memcpy
#define memset fast_memset
#define free fast_free
#define malloc fast_malloc
#define calloc fast_calloc // well not really this

char* _strcpy_s(char* dst, DWORD length, const char* src);
char* _strcat_s(char* dst, DWORD length, const char* src);
void print(const char* str);
inline void* fast_memset(BYTE* dst, BYTE b, DWORD count);
int __itoa(int value, char* sp, int radix);

static HANDLE proc_heap;
static HANDLE stdout;

char* _strcpy(char* dst, const char* src) {
	return _strcpy_s(dst, 0xffffffff, src);
}

char* _strcpy_s(char* dst, DWORD length, const char* src) {

	for (int i = 0; i < length - 1; i++) {
		if (!src[i]) {
			dst[i] = 0;
			return dst;
		}
		dst[i] = src[i];
	}
	dst[length - 1] = 0;
	return dst;
}

char* _strcat(char* dst, const char* src) {
	return _strcat_s(dst, 0xffffffff, src);
}

char* _strcat_s(char* dst, DWORD length, const char* src) {
	DWORD len = strlen(dst);

	for (int i = 0; i < length - len - 1; i++) {
		if (!src[i]) {
			dst[len + i] = 0;
			return dst;
		}
		dst[len + i] = src[i];
	}
	dst[length - 1] = 0;
	return dst;
}

// from stackoverflow
int __itoa(int value, char* sp, int radix)
{
	char tmp[16];// be careful with the length of the buffer
	char* tp = tmp;
	int i;
	unsigned v;

	int sign = (radix == 10 && value < 0);
	if (sign)
		v = -value;
	else
		v = (unsigned)value;

	while (v || tp == tmp)
	{
		i = v % radix;
		v /= radix;
		if (i < 10)
			*tp++ = i + '0';
		else
			*tp++ = i + 'a' - 10;
	}

	int len = tp - tmp;

	if (sign)
	{
		*sp++ = '-';
		len++;
	}

	while (tp > tmp)
		*sp++ = *--tp;

	return len;
}

// modified, from stackoverflow
DWORD __ultoa(DWORD value, char* sp, int radix)
{
	char tmp[26];// be careful with the length of the buffer
	char* tp = tmp;
	DWORD i;
	unsigned v = (unsigned)value;

	while (v || tp == tmp)
	{
		i = v % radix;
		v /= radix;
		if (i < 10)
			*tp++ = i + '0';
		else
			*tp++ = i + 'a' - 10;
	}

	int len = tp - tmp;

	while (tp > tmp)
		*sp++ = *--tp;

	return len;
}

// simple, definitely not all-encompassing snprintf
void v_snprintf(char* buff, DWORD length, const char* format, va_list args) {

	buff[0] = 0; // temp fix

	char c;
	DWORD i = 0;
	while(c = *(format++)) {
		if (c == '%') { // technically goes over length by 1 sometimes
			c = *(format++);
			if (c == 's') {
				char* str = va_arg(args, char*);
				// should be length - i ... TODO: fix!!!!!!!!!!!!!!!!!
				strcat_s(buff, length, str); // could optimise
				i = strlen(buff);
			}
			else if (c == 'd') {
				int num = va_arg(args, int);
				char str[32];
				memset(str, 0, 32);
				itoa(num, str, 10);
				strcat_s(buff, length, str); // could optimise
				i = strlen(buff);
			}
			else if (c == 'L') { // %L == %lu
				DWORD num = va_arg(args, DWORD);
				char str[32];
				memset(str, 0, 32);
				ultoa(num, str, 10);
				strcat_s(buff, length, str); // could optimise
				i = strlen(buff);
			}
			else {
				buff[i++] = '%';
				buff[i++] = c;
				buff[i] = 0; // temp fix
			}
		}
		else {
			buff[i++] = c;
			buff[i] = 0; // temp fix
		}

		if (i >= length) {
			//print("!!!!!!!!!!!!!!!!!!!!\n");
			buff[length - 1] = 0;
			return;
		}

	}
	buff[i] = 0;
}

void snprintf(char* buff, DWORD length, const char* format, ...) {
	va_list args;
	va_start(args, format);
	v_snprintf(buff, length, format, args);
	va_end(args);
}

void sprintf(char* buff, const char* format, ...) {
	va_list args;
	va_start(args, format);
	v_snprintf(buff, 0xffffffff, format, args);
	va_end(args);
}

int stdfuncs_init() {
	proc_heap = GetProcessHeap();
	stdout = GetStdHandle(STD_OUTPUT_HANDLE);
}

// _tolower is a WinAPI macro
int __tolower(char c) {

	int out = c;
	if ('A' <= c && c <= 'Z') {
		out = out - 'A' + 'a';
	}
	return out;
}

void print(const char* str) {
	WriteConsoleA(stdout, str, strlen(str), 0, 0);
}

#define INVALID_NUMBER 2000000000

// atoi but not very big (still bigger than minecraft range)
// TODO: remove unnecessary functionality (maybe idk)
int small_atoi(const char* str) {

	if (!*str) return INVALID_NUMBER;

	// for whitespace, NOT UNNECESSARY!!!
	while (*str == ' ' || *str == '\t') { str++; }

	int x = 0;
	int factor = 1;
	if (*str == '-') {
		factor = -1;
		str++;
	}

	while (*str == ' ' || *str == '\t') { str++; }

	int valid = 0;
	for (int i = 0; i < 9; i++) { // up to 9 digits

		char c = str[i];
		if ('0' <= c && c <= '9') {
			x = x * 10 + c - '0';
			valid = 1;
		}
		else if (!c) break;
		else if (c == ' ' || c == '\t') {
			// could be whitespace until the end
			while (str[i] == ' ' || str[i] == '\t') { i++; }
			//if (!valid) return INVALID_NUMBER;
			if (!str[i]) return x * factor; // ended with just whitespace, OK
			else return INVALID_NUMBER; // invalid number
		} else return INVALID_NUMBER; // ^^^
	}

	if (!valid) return INVALID_NUMBER;
	return x * factor;
}

// TODO: complete
// 1 if we actually read an integer
int get_console_integer(int* num) {

	HANDLE stdin = GetStdHandle(STD_INPUT_HANDLE);

	char buf[32];
	DWORD read;
	// ~ GPT
	if (ReadConsoleA(stdin, buf, sizeof(buf) - 1, &read, NULL))
	{
		buf[read] = 0;

		// remove carriage return + newline
		if (read >= 2 && buf[read - 2] == '\r' && buf[read - 1] == '\n')
		{
			buf[read - 2] = 0;
		}

		if (read > 0) {
			// Still need to check for INVALID_NUMBER (probably)
			*num = small_atoi(buf);
			return 1;
		}
	}
	return 0;
}


int _strcmp(const char* a, const char* b) {

	while (*a && (*a == *b)) {
		a++;
		b++;
	}

	return *(const unsigned char*)a - *(const unsigned char*)b;
}

// from stackoverfow: TODO: maybe change a bit
// dies if a and b are both ""
int strcicmp(const char* a, const char* b)
{
	for (;; a++, b++) {
		int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
		if (d != 0 || !*a)
			return d;
	}
}

int _strlen(const char* str) {

	int length = 0;
	while (*(str++)) {
		length++;
	}
	return length;
}

void printf(const char* format, ...) {

	char* org = format;
	int n = 0;
	char c;
	while (c = *(format++)) {
		if (c == '%') {
			n = 1;
			break;
		}
	}
	format = org;
	
	if (n == 0) {
		print(format);
		return;
	}
	
	char buff[300]; // massively overkill, though there could still be issues
	memset(buff, 0, 300);
	// ^^^ possibly technically fine because of path stuff
	// TODO: streaming print and print that ACTUALLY CALCULATES THE RIGHT SIZE
	va_list args;
	va_start(args, format);
	v_snprintf(buff, 300, format, args);
	va_end(args);
	print(buff);
}

// error msg cannot be longer than 500 chars
void error_print(const char* module_name, const char* func) {
	DWORD err = GetLastError();
	printf("%s %s failed with error %L", module_name, func, err);
}

// these functions actually have to be fast
inline void* fast_memcpy(BYTE* dst, const BYTE* src, DWORD count) {
	__movsb(dst, src, count);
	return dst;
}

inline void* fast_memset(BYTE* dst, BYTE b, DWORD count) {
	__stosb(dst, b, count);
	return dst;
}

int fast_free(void* addr) {
	return HeapFree(proc_heap, 0, addr);
}

void* fast_malloc(DWORD size) {
	return HeapAlloc(proc_heap, 0, size);
}

void* fast_calloc(DWORD num, DWORD size) {
	return HeapAlloc(proc_heap, HEAP_ZERO_MEMORY, num*size);
}