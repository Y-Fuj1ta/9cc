#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>

// トークンの型を表す値
enum {
	TK_NUM = 256, // 整数トークン
	TK_EOF,
};

// ノードの型を表す値
enum {
	ND_NUM = 256,
};

 // トークンの型
typedef struct {
	int ty; // トークンの型
	int val; // tyがTK_NUMの場合、その数値
	char *input; // トークン文字列(エラーメッセージ用)
} Token;

// トークナイズした結果のトークン列はこの配列に保存する
// 100個以上のトークンは来ないものとする
Token tokens[100];

// エラーを報告するための関数
// printfと同じ引数をとる
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// ノードの型
typedef struct Node {
	int ty;
	struct Node *lhs;
	struct Node *rhs;
	int val;
}Node;

// 新しいノードを作成する
Node *new_node(int ty, Node *lhs, Node *rhs) {
	Node *node = malloc(sizeof(Node));
	node->ty = ty;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	Node *node = malloc(sizeof(Node));
	node->ty = ND_NUM;
	node->val = val;
	return node;
}

// 現在着目しているトークンのインデックス
int pos = 0;

// 次のトークンが引数と等しい場合にトークンを読み進めて真を返す
int consume(int ty) {
	if (tokens[pos].ty != ty)
		return 0;
	pos++;
	return 1;
}

Node *add();
Node *mul();

// 和算
Node *add() {
	Node *node = mul();

	for (;;){
		if (consume('+'))
			node = new_node('+', node, mul());
		else if (consume('-'))
			node = new_node('-', node, mul());
		else
			return node;
	}
}

// 乗算
Node *mul(){
	if (tokens[pos].ty != TK_NUM)
		error("数値ではないトークンです： %s", tokens[pos].input);

	Node *node = new_node_num(tokens[pos++].val);
	
	for (;;) {
		if (consume('*'))
			node = new_node('*', node, mul());
		else if (consume('/'))
			node = new_node('/', node, mul());
		else
			return node;
	}
}


// pがさしている文字列をトークンに分割してtokensに保存する
void tokenize (char *p){
	int i = 0;
	while (*p){
		if (isspace(*p)){
			p++;
			continue;
		}
	
		if (*p == '+' || *p == '-' || *p == '*' || *p == '/'){
			tokens[i].ty = *p;
			tokens[i].input = p;
			i++;
			p++;
			continue;
		}
	
		if (isdigit(*p)){
			tokens[i].ty = TK_NUM;
			tokens[i].input = p;
			tokens[i].val = strtol(p, &p, 10);
			i++;
			continue;
		}
	
		error("トークナイズできません： %s", p);
		exit(1);
	}
	tokens[i].ty = TK_EOF;
	tokens[i].input = p;
}

//スタック操作命令
void gen(Node *node){
	if (node->ty == ND_NUM) {
		printf("	push %d\n", node->val);
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("	pop rdi\n");
	printf("	pop rax\n");

	switch (node->ty) {
		case '+':
			printf("	add rax, rdi\n");
			break;
		case '-':
			printf("	sub rax, rdi\n");
			break;
		case '*':
			printf("	mul rdi\n");
			break;
		case '/':
		printf("	mov rdx, 0\n");
		printf("	div rdi\n");
	}
	printf("	push rax\n");
}

int main(int argc, char **argv){
	if (argc != 2){
		fprintf(stderr, "引数の個数が正しくありません。\n");
		return 1;
	}

	// トークナイズしてパースする
	tokenize(argv[1]);
	Node *node = add();

	
	// アセンブリの前半部分を出力
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	// 抽象構文木を下りながらコード生成
	gen(node);

	// スタックトップに敷き全体の値が残っているはずなので
	// それをRAXにロードしてから関数からの返り値とする
	printf("	pop rax\n");
	printf("	ret\n");
	return 0;
}
