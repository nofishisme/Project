#define _CRT_SECURE_NO_WARNINGS

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "scanner.h"

typedef struct {
	const char *start;		// ָ��ǰ����ɨ���Token����ʼ�ַ�
	const char *current;	// �ʷ���������ǰ���ڴ����Token���ַ�,һ��ʼ������start��ʼ,ֱ������������Token,ָ���Token����һ���ַ�
	int line;		// ��¼��ǰToken��������
} Scanner;

// ȫ�ֱ����ṹ�����
static Scanner scanner;

void initScanner(const char *source) {
	// ��ʼ��ȫ�ֱ���Scanner
	scanner.start = source;
	scanner.current = source;
	scanner.line = 1;
}

// �����Ҹ�����ṩ�˺ܶ���õ��ĸ�������,����ʹ��
// �жϵ�ǰ�ַ��ǲ�����ĸ�����»���
static bool isAlpha(char c) {
	return (c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		c == '_';
}
// �жϵ�ǰ�ַ��ǲ�������
static bool isDigit(char c) {
	return c >= '0' && c <= '9';
}
// �ж�Scanner��ǰ���ڴ�����ַ��ǲ��ǿ��ַ�,�ж��ǲ��Ǵ�������
static bool isAtEnd() {
	return *scanner.current == '\0';
}

// currָ��ǰ��һ���ַ�,������֮ǰcurrָ��ָ���Ԫ��
static char advance() {
	return *scanner.current++;
}
// �鿴��ǰ���ڴ�����ַ���ʲô,curr����
static char peek() {
	return *scanner.current;
}
// �����ǰ���ڴ�����ַ����ǿ��ַ�,�Ǿ�Ƴһ����һ���ַ���ʲô,curr����
static char peekNext() {
	if (isAtEnd()) {
		return '\0';
	}
	return *(scanner.current + 1);
}

// �жϵ�ǰ���ڴ�����ַ��ǲ��Ƿ���Ԥ��,�������currǰ��һλ
static bool match(char expected) {
	if (isAtEnd()) {
		return false;	// ������ڴ�����ǿ��ַ�,�Ǿͷ���false
	}
	if (peek() != expected) {
		return false;	// ���������Ԥ��,Ҳ����false
	}
	scanner.current++;
	return true;	// ֻ�з���Ԥ�ڲŻ᷵��true ����curr��ǰ��һλ
}

// ���ݴ����TokenType���������췵��һ��Token
static Token makeToken(TokenType type) {
	int len = (int)(scanner.current - scanner.start);// ����Token�ַ����ĳ���
	Token token;
	token.type = type;
	token.start = malloc(len * sizeof(char));
	//token.start = scanner.start;
	if (!token.start) {
		printf("makeToken malloc error.\n");
		exit(EXIT_FAILURE);
	}
	memcpy(token.start, scanner.start, len);
	token.length = len;
	//token.length = (int)(scanner.current - scanner.start);// ����Token�ַ����ĳ���
	token.line = scanner.line;
	return token;
}

// �������ܽ��������ʱ�����Ǵ���һ��ERROR Token. ���磺����@��$�ȷ���ʱ�������ַ������ַ�û�ж�Ӧ��������ʱ��
static Token errorToken(const char *message) {
	int len = (int)strlen(message);// ����Token�ַ����ĳ���
	Token token;
	token.type = TOKEN_ERROR;
	//token.start = message;
	token.start = malloc(len * sizeof(char));
	if (!token.start) {
		printf("errorToken malloc error.\n");
		exit(EXIT_FAILURE);
	}
	memcpy(token.start, message, len);
	token.length = len;
	//token.length = (int)strlen(message);
	token.line = scanner.line;
	return token;
}

static void skipWhitespace() {
	// �����հ��ַ�: ' ', '\r', '\t', '\n'��ע��
	// ע����'//'��ͷ, һֱ����β
	// ע�����scanner.line��

	// ��ʾ: ����������Ҫ�����м�ĺܶ��ַ�,������Ҫ��ѭ��
	while (1) {
		switch (*scanner.current)
		{
		case ' ':
		case '\r':
		case '\t':
			advance();
			break;
		case '\n':
			advance();
			scanner.line++;
			break;
		case '/':
			//������ע��//
			if (peekNext() == '/') {
				while (peek() != '\n') {
					advance();
				}
			}
			//�������ע��/**/
			else if (peekNext() == '*') {
				while (peek() != '*' || (peek() == '*' && peekNext() != '/')) {
					if (peek() == '\n') {
						scanner.line++;
					}
					advance();
				}
				advance();
				advance();
			}
			break;
		default:
			scanner.start = scanner.current;
			return;
			break;
		}
	}
}

// ���ڼ�鵱ǰɨ���Token�������ǲ���type ����Ǿͷ���type
static TokenType checkKeyword(int start, int length, const char *rest, TokenType type) {
	/*
		start: ��������е���ʼ�ַ��±�
			����Ҫ���ؼ���break����ô��case b��ǰ���£���Ҫ����reak�����м��
			����start�͵���1��scanner.start[1]
		length: ��������еĳ��ȣ����������break�����Ǽ��ʣ���reak
			��Ҫ����4
		restָ�룬������ʣ�������ַ���������ֱ�Ӵ���һ������ֵ�ַ���������
			������break������"reak"�ͺ���
		type����Ҫ���Ĺؼ���Token�����ͣ�������break���Ǿʹ���Token_BREAK
	*/
	if (scanner.current - scanner.start == start + length &&
		/*
					int memcmp(const void *s1, const void *s2, size_t n);
					����Ĳ����ֱ��ǣ�

					s1��ָ���һ���ڴ������ָ�롣
					s2��ָ��ڶ����ڴ������ָ�롣
					n��Ҫ�Ƚϵ��ֽ�����
					memcmp ���������ֽڱȽ� s1 �� s2 ָ����ڴ�����ֱ���в���ȵ��ֽڻ�Ƚ��� n ���ֽ�Ϊֹ��
					��������ڴ�������ȫ��ͬ��
					�� memcmp ���� 0�������һ����ͬ���ֽ��� s1 �е�ֵС�� s2 �ж�Ӧ��ֵ�����ظ�����
					��֮������������
		*/
		memcmp(scanner.start + start, rest, length) == 0) {
		return type;
	}
	return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
	// ȷ��identifier������Ҫ�����ַ�ʽ��
	// 1. �����еĹؼ��ַ����ϣ���У�Ȼ����ȷ�� 
	// Key-Value ����"�ؼ���-TokenType" ������ �����ڶ����ڴ�ռ����Ч�ʲ�����һ����ʽ��
	// 2. �����еĹؼ��ַ���Trie��(�ֵ������)�У�Ȼ����ȷ��
	// Trie���ķ�ʽ�����ǿռ��ϻ���ʱ���϶����ڹ�ϣ��ķ�ʽ
	// ��switch...switch...if��Ϲ����߼��ϵ�trie��
	char first = scanner.start[0];	// ��Token�ĵ�һ���ַ�
	switch (first) {
	case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
	case 'c': {
		char second = scanner.start[1];
		switch (second)
		{
		case 'a': return checkKeyword(2, 2, "se", TOKEN_CASE);
		case 'h': return checkKeyword(2, 2, "ar", TOKEN_CHAR);
		case 'o': {
			char fourth = scanner.start[3];
			switch (fourth)
			{
			case 's': return checkKeyword(4, 1, "t", TOKEN_CONST);
			case 't': return checkKeyword(4, 4, "inue", TOKEN_CONTINUE);
			default:
				break;
			}
		}
		default:
			break;
		}
	}
	case 'd': {
		char second = scanner.start[1];
		switch (second)
		{
		case 'e': return checkKeyword(2, 5, "fault", TOKEN_DEFAULT);
		case 'o': {
			char third = scanner.start[2];
			switch (third)
			{
			case 'u': return checkKeyword(3, 3, "ble", TOKEN_DOUBLE);
			default:
				return checkKeyword(1, 1, "o", TOKEN_DO);
			}
		}
		default:
			break;
		}
	}
	case 'e': {
		char second = scanner.start[1];
		switch (second)
		{
		case 'l': return checkKeyword(2, 2, "se", TOKEN_ELSE);
		case 'n': return checkKeyword(2, 2, "un", TOKEN_ENUM);
		default:
			break;
		}
	}
	case 'f': {
		char second = scanner.start[1];
		switch (second)
		{
		case 'l': return checkKeyword(2, 3, "oat", TOKEN_FLOAT);
		case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
		default:
			break;
		}
	}
	case 'g': return checkKeyword(1, 2, "to", TOKEN_GOTO);
	case 'i': {
		char second = scanner.start[1];
		switch (second)
		{
		case 'f': return checkKeyword(1, 1, "f", TOKEN_IF);
		case 'n': return checkKeyword(2, 1, "t", TOKEN_INT);
		default:
			break;
		}
	}
	case 'l': return checkKeyword(1, 3, "ong", TOKEN_LONG);
	case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
	case 's': {
		char second = scanner.start[1];
		switch (second)
		{
		case 'h': return checkKeyword(2, 3, "ort", TOKEN_SHORT);
		case 'i': {
			char third = scanner.start[2];
			switch (third)
			{
			case 'g': return checkKeyword(3, 3, "ned", TOKEN_SIGNED);
			case 'z': return checkKeyword(3, 3, "eof", TOKEN_SIZEOF);
			default:
				break;
			}
		}
		case 't': return checkKeyword(2, 4, "ruct", TOKEN_STRUCT);
		case 'w': return checkKeyword(2, 4, "itch", TOKEN_SWITCH);
		default:
			break;
		}
	}
	case 't': return checkKeyword(1, 6, "ypedef", TOKEN_TYPEDEF);
	case 'u': {
		char second = scanner.start[1];
		switch (second)
		{
		case 'n': return checkKeyword(2, 3, "ion", TOKEN_UNION);
		case 'i': return checkKeyword(2, 5, "nsigned", TOKEN_UNSIGNED);
		default:
			break;
		}
	}
	case 'v': return checkKeyword(1, 3, "oid", TOKEN_VOID);
	case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
	}
	// û�н�switchһ���Ǳ�ʶ��
	return TOKEN_IDENTIFIER;
}

// ��ǰToken�Ŀ�ͷ���»��߻���ĸ�ж����ǲ��Ǳ�ʶ��Token
static Token identifier() {
	// �ж�currָ�뵱ǰ���ڴ�����ַ��ǲ��� ��ĸ �»��� ����
	while (isAlpha(peek()) || isDigit(peek())) {
		advance();  // ����ǰ������һ���ַ� ֱ��������һ���ַ�������ĸ �»��� �Լ����� ����Token
	}
	// ��whileѭ������ʱ��currָ��ָ����Ǹ�Token�ַ�������һ���ַ�
	// �����������˼��: ֻҪ������ĸ���»��߿�ͷ��Token���Ǿͽ����ʶ��ģʽ
	// Ȼ��һֱ�ҵ���Token��ĩβ
	// ���������е����ﻹ��ȷ��Token�Ǳ�ʶ�����ǹؼ���, ��Ϊ��������break, var, goto, max_val...
	// ����ִ��identifierType()��������������ȷ��Token���͵�
	return makeToken(identifierType());
}

static Token number() {
	// ����������ǽ�NUMBER�Ĺ���������:
	// 1. NUMBER���԰������ֺ����һ��'.'��
	// 2. '.'��ǰ��Ҫ������
	// 3. '.'�ź���ҲҪ������
	// ��Щ���ǺϷ���NUMBER: 123, 3.14
	// ��Щ���ǲ��Ϸ���NUMBER: 123., .14(��Ȼ��C�����кϷ�)
	// ��ʾ: ���������Ҫ���ϵ������м�����ְ���С����,����Ҳ��Ҫѭ��
	int point_count = 0;
	while (isDigit(peek()) || peek() == '.') {
		if (peek() == '.') {
			point_count++;
		}
		advance();
	}
	return point_count > 1 ? errorToken("Too much point") : makeToken(TOKEN_NUMBER);
}

static Token string() {
	// �ַ�����"��ͷ����"��β�����Ҳ��ܿ���
	// Ϊ�˼򻯹�����,�����ַ���
	// �����һ���ַ�����ĩβҲ����˫���ţ�ȫ������(curr���Լ�¼���ȣ����õ���)
	while (peek() != 0 && peek() != '"' && peek() != '\n') {
		//��������\"
		if (peek() == '\\' && peekNext() == '"') {
			advance();
		}
		advance();
	}
	char ch = advance();
	return ch == '"' ? makeToken(TOKEN_STRING) : errorToken("Not a string");
}

static Token character() {
	// �ַ�'��ͷ����'��β�����Ҳ��ܿ���
	// �����һ���ַ�����ĩβҲ���ǵ����ţ�ȫ������(curr���Լ�¼���ȣ����õ���)
	// ��������������˵һģһ����Ҳ�Ǽ���һ��
	while (peek() != 0 && peek() != '\'' && peek() != '\n') {
		advance();
	}
	char ch = advance();
	return ch == '\'' ? makeToken(TOKEN_CHARACTER) : errorToken("Not a character");
}

// �����޷�ʶ����ַ�
static Token errorTokenWithChar(char character) {
	char message[50];
	// ���޷�ʶ����ַ���ʲô���
	sprintf(message, "Unexpected character: %c", character);
	return errorToken(message);
}

// Scanner�����߼�,���ڷ���һ�������õ�Token
Token scanToken() {
	// ����ǰ�ÿհ��ַ���ע��
	skipWhitespace();
	// ��¼��һ��Token����ʼλ��
	scanner.start = scanner.current;

	// ����Ѿ���������� ֱ�ӷ���TOKEN_EOF
	if (isAtEnd()) {
		return makeToken(TOKEN_EOF);
	}

	// currָ������ָ��Token�ĵڶ����ַ�,������ַ�c��Ȼ�ǵ�һ���ַ���ֵ
	char c = advance();
	//printf("%c", c);

	// ���Token�ĵ�һ���ַ�����ĸ���»��߾ͽ����ʶ���Ĵ���ģʽ
	if (isAlpha(c)) {
		return identifier();
	}
	// ���Token�ĵ�һ���ַ�������,�Ǿͽ������ֵĴ���ģʽ
	if (isDigit(c)) {
		return number();
	}
	// ���Token�ĵ�һ���ַ��Ȳ�������Ҳ������ĸ���»���,��ô��switch������
	switch (c) {
		// ��һ����: �����ַ�Token
	case '(': return makeToken(TOKEN_LEFT_PAREN);
	case ')': return makeToken(TOKEN_RIGHT_PAREN);
	case '[': return makeToken(TOKEN_LEFT_BRACKET);
	case ']': return makeToken(TOKEN_RIGHT_BRACKET);
	case '{': return makeToken(TOKEN_LEFT_BRACE);
	case '}': return makeToken(TOKEN_RIGHT_BRACE);
	case ',': return makeToken(TOKEN_COMMA);
	case '.': return makeToken(TOKEN_DOT);
	case ';': return makeToken(TOKEN_SEMICOLON);
	case '~': return makeToken(TOKEN_TILDE);
			// �ɵ���˫�ַ���Token�������΢����һ��,������
			// ���Token�ĵ�һ���ַ���+��
	case '+':
		// ���Token�ĵڶ����ַ�Ҳ��+,�Ǿ�����++˫�ַ�Token����
		if (match('+')) {
			return makeToken(TOKEN_PLUS_PLUS);
		}
		else if (match('=')) {
			return makeToken(TOKEN_PLUS_EQUAL);
		}
		return makeToken(TOKEN_PLUS);
	case '-':
		// ���Token�ĵڶ����ַ�Ҳ��-,�Ǿ�����--˫�ַ�Token����
		if (match('-')) {
			return makeToken(TOKEN_MINUS_MINUS);
		}
		else if (match('=')) {
			return makeToken(TOKEN_MINUS_EQUAL);
		}
		else if (match('>')) {
			return makeToken(TOKEN_MINUS_GREATER);
		}
		return makeToken(TOKEN_MINUS);
	case '*':
		// ���Token�ĵڶ����ַ�Ҳ��=,�Ǿ�����*=˫�ַ�Token����
		if (match('=')) {
			return makeToken(TOKEN_STAR_EQUAL);
		}
		return makeToken(TOKEN_STAR);
	case '/':
		// ���Token�ĵڶ����ַ�Ҳ��=,�Ǿ�����/=˫�ַ�Token����
		if (match('=')) {
			return makeToken(TOKEN_SLASH_EQUAL);
		}
		return makeToken(TOKEN_SLASH);
	case '%':
		// ���Token�ĵڶ����ַ�Ҳ��=,�Ǿ�����%=˫�ַ�Token����
		if (match('=')) {
			return makeToken(TOKEN_PERCENT_EQUAL);
		}
		return makeToken(TOKEN_PERCENT);
	case '&':
		// ���Token�ĵڶ����ַ�Ҳ��=,�Ǿ�����&=˫�ַ�Token����
		if (match('=')) {
			return makeToken(TOKEN_AMPER_EQUAL);
		}
		else if (match('&')) {
			return makeToken(TOKEN_AMPER_AMPER);
		}
		return makeToken(TOKEN_AMPER);
	case '|':
		// ���Token�ĵڶ����ַ�Ҳ��=,�Ǿ�����|=˫�ַ�Token����
		if (match('=')) {
			return makeToken(TOKEN_PIPE_EQUAL);
		}
		else if (match('|')) {
			return makeToken(TOKEN_PIPE_PIPE);
		}
		return makeToken(TOKEN_PIPE);
	case '^':
		// ���Token�ĵڶ����ַ�Ҳ��=,�Ǿ�����^=˫�ַ�Token����
		if (match('=')) {
			return makeToken(TOKEN_HAT_EQUAL);
		}
		return makeToken(TOKEN_HAT);
	case '=':
		// ���Token�ĵڶ����ַ�Ҳ��=,�Ǿ�����==˫�ַ�Token����
		if (match('=')) {
			return makeToken(TOKEN_EQUAL_EQUAL);
		}
		return makeToken(TOKEN_EQUAL);
	case '!':
		// ���Token�ĵڶ����ַ�Ҳ��=,�Ǿ�����!=˫�ַ�Token����
		if (match('=')) {
			return makeToken(TOKEN_BANG_EQUAL);
		}
		return makeToken(TOKEN_BANG);
	case '<':
		// ���Token�ĵڶ����ַ�Ҳ��=,�Ǿ�����<=˫�ַ�Token����
		if (match('=')) {
			return makeToken(TOKEN_LESS_EQUAL);
		}
		else if (match('<')) {
			return makeToken(TOKEN_LESS_LESS);
		}
		return makeToken(TOKEN_LESS);
	case '>':
		// ���Token�ĵڶ����ַ�Ҳ��=,�Ǿ�����>=˫�ַ�Token����
		if (match('=')) {
			return makeToken(TOKEN_GREATER_EQUAL);
		}
		else if (match('>')) {
			return makeToken(TOKEN_GREATER_GREATER);
		}
		return makeToken(TOKEN_GREATER);
	case '"': return string(); // ���Token�ĵ�һ���ַ���˫����,�Ǿͽ������ַ���Token����
	case '\'': return character();	// ���Token�ĵ�һ���ַ��ǵ�����,�Ǿͽ������ַ�Token����
	}
	// �����������û�д���ɹ�,��û�����ɺ��ʵ�Token,˵�����ַ��޷�ʶ��
	return errorTokenWithChar(c);
}
