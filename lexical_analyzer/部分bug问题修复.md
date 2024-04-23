### 部分bug问题修复

### 1.添加处理注释: /**/

​	增加处理跳过多行注释的情况

```c
static void skipWhitespace() {
	// 跳过空白字符: ' ', '\r', '\t', '\n'和注释
	// 注释以'//'开头, 一直到行尾
	// 注意更新scanner.line！

	// 提示: 整个过程需要跳过中间的很多字符,所以需要死循环
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
            //处理单行注释
			if (peekNext() == '/') {
				while (peek() != '\n') {
					advance();
				}
			}
            //处理多行注释/**/
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
```

### 2.处理字符串中存在`\"`的情况

```c
static Token string() {
	// 字符串以"开头，以"结尾，而且不能跨行
	// 为了简化工作量,简化了字符串
	// 如果下一个字符不是末尾也不是双引号，全部跳过(curr可以记录长度，不用担心)
	while (peek() != 0 && peek() != '"' && peek() != '\n') {
        //处理跳过\"
		if (peek() == '\\' && peekNext() == '"') {
			advance();
		}
		advance();
	}
	char ch = advance();
	return ch == '"' ? makeToken(TOKEN_STRING) : errorToken("Not a string");
}
```

### 3.处理第一行是#define输出不符合预期的问题

​	问题描述：

​	当第一行是带有#号等未识别字符时，报错信息不符合预期。但是后续处理的报错信息正常。

![image-20240423110931689](./%E9%83%A8%E5%88%86bug%E9%97%AE%E9%A2%98%E4%BF%AE%E5%A4%8D/image-20240423110931689.png)

​	问题解决：

​	检查报错处理时用于Token生成的函数`errorToken`代码，发现存在字符串浅拷贝局部变量地址导致`token.start`指向一个已经销毁的未定义内存区。后续进行token打印时存在未定义的访问行为。

​	**产生报错的原代码：**

```c
// 处理无法识别的字符
static Token errorTokenWithChar(char character) {
	char message[50];
	// 将无法识别的字符是什么输出
	sprintf(message, "Unexpected character: %c", character);
	return errorToken(message);
}
```

```c
// 遇到不能解析的情况时，我们创建一个ERROR Token. 比如：遇到@，$等符号时，比如字符串，字符没有对应的右引号时。
static Token errorToken(const char *message) {
	Token token;
	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = (int)strlen(message);
	token.line = scanner.line;
	return token;
}
```

​	可以看到`token.start`得到的赋值是来自函数`errorTokenWithChar`的局部变量数组`message`的地址。当函数`errorTokenWithChar`运行结束时，`message`这个局部变量数组已经被销毁，此时再使用该数组的地址进行访问是未定义行为，输出就不确定。

​	本次修复解决办法是使用堆空间进行内存分配，直接进行深度拷贝`message`的内容。需要注意使用完`token`后需要进行内存释放，否则会造成内存泄漏。

​	**修改后的代码：**

变更：`const  char *start;` --->`char *start;`

```c
typedef struct {
    TokenType type;		// Token的类型, 取任一枚举值
    // Token的起始字符指针
    char *start;	// start指向source中的字符，source为读入的源代码。
    int length;		    // length表示这个Token的长度
    int line;		    // line表示这个Token在源代码的哪一行, 方便后面的报错和描述Token
} Token;	// 这个Token只涉及一个字符指针指向源代码的字符信息,没有在内部保存字符数据
```

变更：使用堆空间分配内存

```c
// 遇到不能解析的情况时，我们创建一个ERROR Token. 比如：遇到@，$等符号时，比如字符串，字符没有对应的右引号时。
static Token errorToken(const char *message) {
	int len = (int)strlen(message);// 计算Token字符串的长度
	Token token;
	token.type = TOKEN_ERROR;
	token.start = malloc(len * sizeof(char));
	if (!token.start) {
		printf("errorToken malloc error.\n");
		exit(EXIT_FAILURE);
	}
	memcpy(token.start, message, len);
	token.length = len;
	token.line = scanner.line;
	return token;
}
```

变更：使用堆空间分配内存

```c
// 根据传入的TokenType类型来制造返回一个Token
static Token makeToken(TokenType type) {
	int len = (int)(scanner.current - scanner.start);// 计算Token字符串的长度
	Token token;
	token.type = type;
	token.start = malloc(len * sizeof(char));
	if (!token.start) {
		printf("makeToken malloc error.\n");
		exit(EXIT_FAILURE);
	}
	memcpy(token.start, scanner.start, len);
	token.length = len;
	token.line = scanner.line;
	return token;
}
```

变更：`run`函数中每次输出完`token`后需要`free`分配的内存

```c
// main.c中的核心逻辑
static void run(const char *source) {
	initScanner(source);	// 初始化词法分析器
	int line = -1;  // 用于记录当前处理的行号,-1表示还未开始解析//FILE* ofs = fopen("1.txt", "w");
	for (;;) {  
		Token token = scanToken();  // 获取下一个TOKEN
		if (token.line != line) {		// 如果Token中记录行和现在的lin不同就执行换行打印的效果		
			printf("%4d ", token.line); 
			line = token.line; 
		}
		else {		// 没有换行的打印效果
			printf("   | ");
		}
		char *str = convert_to_str(token);
		printf("%s '%.*s'\n", str, token.length, token.start);
		//每次使用完token，就需要把token分配的字符串堆空间释放
		free(token.start);

		if (token.type == TOKEN_EOF) {
			break;	// 读到TOKEN_EOF结束循环
		}
	}
}
```

![正常输出](./%E9%83%A8%E5%88%86bug%E9%97%AE%E9%A2%98%E4%BF%AE%E5%A4%8D/image-20240423113417650.png)