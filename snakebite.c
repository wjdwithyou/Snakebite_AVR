#include <avr/io.h>
#include <avr/interrupt.h> // ���ͷ�Ʈ�� ����ϱ� ����
#include <stdio.h> // for sprintf()
#include <stdlib.h> // for srand()

#define LEFT 75 // �̵�����
#define RIGHT 77
#define UP 72
#define DOWN 80

#define MAX_SIZE 8 // Dotmatrix�� ũ��
#define MAX2_SIZE 64 // Dotmatrix^2�� ũ��

#define COM_DDR	DDRF // Dotmatrix
#define COM_PORT PORTF
#define RED_DDR DDRA
#define RED_PORT PORTA
#define GREEN_DDR DDRC
#define GREEN_PORT PORTC

#define LCD_DDR DDRB // TextLCD
#define LCD_PORT PORTB
#define LCOM_DDR DDRG
#define LCOM_PORT PORTG
#define LCD_RS 0x01
#define LCD_RW 0x02
#define LCD_E 0x04

#define LED1_DDR DDRE // LED
#define LED1_PORT PORTE
#define LED2_DDR DDRD
#define LED2_PORT PORTD

enum BOOL{FALSE, TRUE}; // bool Ÿ���� �������� �ʾƼ� enum�� �̿��� �ӽ÷� ���
enum STATE{READY_STATE, GAME_STATE, END_STATE}; // ������ ����

typedef struct SNAKE {
	int x;
	int y;
	int length;
	int dir;
}SNAKE; // �� ����ü





/****************************************
**************���� ���� ����**************
****************************************/

int cnt = 0; // for OVERFLOW0
int led = 0; // for LED
int speed = 50; // ���忡 ���� ������ �ӵ��� �������� �ϱ����� ����

int state, score, last_dir;
int is_feed; // BOOL
SNAKE snake;
char score_str[16];
int course_x[MAX2_SIZE], course_y[MAX2_SIZE]; // ������ ��θ� �����Ѵ�.
int matrix[MAX_SIZE][MAX_SIZE];

/****************************************/





/****************************************
**********�Լ��� ������Ÿ�� ����**********
****************************************/

SIGNAL(SIG_OVERFLOW0); // ���ͷ�Ʈ
SIGNAL(SIG_INTERRUPT0);
SIGNAL(SIG_INTERRUPT3);
SIGNAL(SIG_INTERRUPT7);
SIGNAL(SIG_INTERRUPT4);
SIGNAL(SIG_INTERRUPT5);

void initTimer(); // Ÿ�̸�

void enablePulse(); // TextLCD
void initLCD();
void sendLCDcommand(unsigned char);
void sendLCDdata(unsigned char);
void dispString(char*);
void locate(int, int);

void setLED(); // LED

void snakeInit(); // ���� ������ �ʿ��� �Լ�
void matrixInit();
void courseInit();
void makeFeed();
void input();
void move();
void draw();
int collisionCheck(); // BOOL
int reverseCheck(); // BOOL
void feedCheck();
void printText();

void us_delay(unsigned short); // delay
void ms_delay(unsigned short);

/****************************************/





int main() {
	// �� ��Ʈ�� ����°� �ʱⰪ ����
	COM_DDR	= 0xFF;
	COM_PORT = 0x00;
	
	RED_DDR	= 0xFF;
	RED_PORT = 0x00;

	GREEN_DDR =	0xFF;
	GREEN_PORT = 0x00;

	LCD_DDR = 0xFF; 
	LCD_PORT = 0x00;

  	LCOM_DDR = 0xFF;
  	LCOM_PORT = 0x00;

	// PE3~PE0 
	LED1_DDR = 0x0F; // 00001111
	LED1_PORT = 0x00;

	// PD7~PD4 
	LED2_DDR = 0xF0; // 11110000
	LED2_PORT = 0x00;

	initTimer();
 
	EICRA = 0x82; // 10000010
	EICRB = 0x8A; // 10001010

	EIMSK = 0xB9; // 10111001
	
	srand(TCNT0);
	// time.h�� ��� time() �Լ��� �� �� ������.
	// ���, time() �Լ�ó�� ����ؼ� ���� ����Ǵ� TCNT0�� ����ߴ�.

	sei(); // ��ü ���ͷ�Ʈ �㰡
	initLCD();

	state = READY_STATE;
	printText();

	while(1) { // ��� �ݺ��Ѵ�.
		switch(state) { // ���� ���¿� ���� �����Ѵ�.
		case READY_STATE: 
			draw(); // �ݺ��ؼ� ������ �׸���.
			break;

		case GAME_STATE:
			snakeInit(); // �� �ʱ�ȭ
			matrixInit(); // �� �ʱ�ȭ
			courseInit(); // ��θ� ������ ť �ʱ�ȭ

			is_feed = FALSE; // �ʱⰪ���� ���̰� ������ �����ؾ� ���̻��� �Լ��� ���� ���̰� ������ ��ġ�� �����ȴ�.
			score = 0; // �ʱ� ����
			speed = 50; // �ʱ� �ӵ�, ���� �۾������� ��������. �ڼ��� ������ SIGNAL(SIG_OVERFLOW0)�� ����.
			
			while(state==GAME_STATE)
				draw(); // �ݺ��ؼ� ������ �׸���.

			break;

		case END_STATE:	
			draw(); // �ݺ��ؼ� ������ �׸���.
			break;

		default:
			break;
		}
	}

	return 0;
}

SIGNAL(SIG_OVERFLOW0) {
	cli();

	TCNT0 = 112; // 0.01�ʸ��� ���ͷ�Ʈ �߻�
	++cnt;
	
	if(cnt==speed) { // cnt�� speed�� ������ ������ �����Ѵ�. speed�� ���� ���� ���̸� ���� ������ �����ϹǷ�, �ᱹ ������ ���� �������� �ȴ�.
		cnt = 0;

		if(led==128) // LED ����� ���� ���� ����
			led = 1;
		else
			led <<= 1;
		
		if(state==GAME_STATE) {
			move(); // ���� �����̰�
			setLED(); // LED�� ��ȭ��Ų��.

			if(!is_feed) // ���̰� ������
				makeFeed(); // ���̸� �����.
		}
	}

	sei();
}

SIGNAL(SIG_INTERRUPT0) { // ENTER
	switch(state) {
	case READY_STATE: // �غ�����϶�
		state = GAME_STATE; // ���ӻ��·� �ٲ۴�.
		setLED();
		printText();
		break;

	case GAME_STATE: // ���� ���϶��� ��ȭ����
		break;

	case END_STATE: // ��������϶�
		state = READY_STATE; // �غ���·� �ٲ۴�.
		setLED();
		printText();
		break;
		
	default:
		break;
	}
}

SIGNAL(SIG_INTERRUPT3) { // LEFT
	if(state==GAME_STATE) { // ���� ���϶��� �۵��Ѵ�.
		last_dir = snake.dir; // ���� ���� �̵� ������ ������ �����ϰ�
		snake.dir = LEFT; // ���� �̵� ������ �Է��� Ű�� �°� ��ȭ��Ų��.
	
		if(snake.length!=1 && reverseCheck(last_dir, snake.dir)) { // ���� ���̰� 2�̻��̰�, ������ �̵������ �ݴ�Ǵ� ������ Ű�� �����ٸ�
			state = END_STATE; // ���ӿ���
			setLED();
			printText();
		}
	} 
}

SIGNAL(SIG_INTERRUPT7) { // RIGHT
	if(state==GAME_STATE) {
		last_dir = snake.dir;
		snake.dir = RIGHT;

		if(snake.length!=1 && reverseCheck(last_dir, snake.dir)) {
			state = END_STATE;
			setLED();
			printText();
		}
	}

}

SIGNAL(SIG_INTERRUPT4) { // UP
	if(state==GAME_STATE) {
		last_dir = snake.dir;
		snake.dir = UP;
	
		if(snake.length!=1 && reverseCheck(last_dir, snake.dir)) {
			state = END_STATE;
			setLED();
			printText();
		}
	}
}

SIGNAL(SIG_INTERRUPT5) { // DOWN
	if(state==GAME_STATE) {
		last_dir = snake.dir;
		snake.dir = DOWN;
	
		if(snake.length!=1 && reverseCheck(last_dir, snake.dir)) {
			state = END_STATE;
			setLED();
			printText();
		}
	}
}

void initTimer() { // Ÿ�̸�, ī���� ����
	TCCR0 =	0x07;	
	TCNT0 =	112;
	TIMSK =	0x01;	
	TIFR = 0x00;

	return;
}





/****************************************
************TextLCD ���� �Լ�************
****************************************/

void enablePulse() {
    LCOM_PORT |= LCD_E;
    asm("NOP");
    LCOM_PORT &= ~LCD_E;

	return;
}

void sendLCDcommand(unsigned char command) {
	LCOM_PORT &= ~LCD_RS;
  	LCD_PORT = command;

  	enablePulse();

  	us_delay(46);

	return;
}

void initLCD() {
  	ms_delay(30);

  	LCOM_PORT &= ~LCD_RW;

  	sendLCDcommand(0x38);
  	sendLCDcommand(0x0C);
  	sendLCDcommand(0x01);

  	ms_delay(2);

	return;
}

void sendLCDdata(unsigned char data) {
  	LCOM_PORT |= LCD_RS;
  	LCD_PORT = data;

  	enablePulse();

  	us_delay(46);

	return;
}

void dispString(char* str) {
   	while(*str) {
		sendLCDdata(*str);
      	str++;
   	}

	return;
}

void locate(int x, int y) {
  	unsigned char RamAddr;

  	if(!y)
		RamAddr = 0x80 + x;
  	else
		RamAddr = 0xC0 + x;
  	
	sendLCDcommand(RamAddr);

	return;
}

/****************************************/





void setLED() { // ������ ���¿� ���� LED���
	switch(state) {
	case READY_STATE: // �غ� �����϶�
		LED1_PORT = 0x00;
		LED2_PORT = 0x00; // ��� ������.

		break;

	case GAME_STATE: // ���� ���϶�
		switch(led) { // led ���� ���� �°� ����Ѵ�.
		case 1:
			LED1_PORT = 0x01;
			LED2_PORT = 0x00;
			break;
		case 2:
			LED1_PORT = 0x02;
			LED2_PORT = 0x00;
			break;
		case 4:
			LED1_PORT = 0x04;
			LED2_PORT = 0x00;
			break;
		case 8:
			LED1_PORT = 0x08;
			LED2_PORT = 0x00;
			break;
		case 16:
			LED1_PORT = 0x00;
			LED2_PORT = 0x10;
			break;
		case 32:
			LED1_PORT = 0x00;
			LED2_PORT = 0x20;
			break;
		case 64:
			LED1_PORT = 0x00;
			LED2_PORT = 0x40;
			break;
		case 128:
			LED1_PORT = 0x00;
			LED2_PORT = 0x80;
			break;
		default:
			break;
		}

		break;

	case END_STATE: // ���� �����϶�
		LED1_PORT = 0x0F;
		LED2_PORT = 0xF0; // ����� LED�� ��� �Ҵ�.

		break;

	default:
		break;
	}

	return;
}

void snakeInit() { // �� �ʱ�ȭ
	snake.x = snake.y = 0; // ���� �ʱ� ��ġ�� (0, 0)
	snake.length = 1; // ���� �ʱ� ���̴� 1
	snake.dir = RIGHT; // ���� �ʱ� ������ ������

	return;
}

void matrixInit() { // �� �ʱ�ȭ
	int i, j;

	for(i=0;i<MAX_SIZE;i++) {
		for(j=0;j<MAX_SIZE;j++) {
			if(i==0&&j==0)
				matrix[i][j] = 2; // 2�� ���� �Ӹ��� ��Ÿ����.
			else
				matrix[i][j] = 0; // 0�� �ƹ��͵� ������ ��Ÿ����.
		}
	}

	return;
}

void courseInit() { // ��� �ʱ�ȭ
	int i;

	course_x[0] = snake.x;
	course_y[0] = snake.y; // ť�� ���� �ʱ� ��ġ�� �ִ´�.

	for(i=1;i<MAX2_SIZE;i++)
		course_x[i] = course_y[i] = -1; // �������� -1�� �ʱ�ȭ

	return;
}

void makeFeed() { // ���� ����
	int x, y;

	while(1) {
		x = rand() % 8 + 1;
		y = rand() % 8 + 1; // ������ x, y���� �����Ѵ�.
			
		if(matrix[y][x]==0) { // (x, y) ��ġ�� �ƹ��͵� ���ٸ�
			matrix[y][x] = 3; // (x, y) ��ġ�� ���̸� �����Ѵ�. 3�� ���̸� �ǹ��Ѵ�.
			break;
		}
	}

	is_feed = TRUE; // ���� Ȯ�� ������ ������ �ٲ۴�. (���� ����)

	return;
}

void move() { // �̵�
	register int i;

	for(i=0;i<snake.length;i++) {
		if(i==snake.length-1)
			matrix[course_y[i]][course_x[i]] = 0; // ���� �����κ��� 0����
		else
			matrix[course_y[i]][course_x[i]] = 1; // �������� 1���� �ٲ۴�.
	}

	// �� ������ ���� �Ӹ��� 2, ������ 1, �� ���� �� ������ 0�̶� ���� ��, 
	// 000000000211111111111000000000
	// 000000002111111111110000000000
	// �� ���� ���ϱ� �����̴�. (���� �̵��Կ� ���� ������ �ִ� ���� 0���� �ٲ�)
	// (�Ӹ��κ��� �Լ��� �Ʒ��ʿ��� ���� �����.)

	switch(snake.dir) { // ���� �̵� ���⿡ ���� �Ӹ��� ��ġ�� ��ȭ��Ų��.
	case LEFT:
		--snake.x;
		break;

	case RIGHT:
		++snake.x;
		break;

	case UP:
		--snake.y;
		break;

	case DOWN:
		++snake.y;
		break;

	default:
		break;
	}

	if(collisionCheck()) { // ���̳� �ڽ��� ����� �浹�ߴٸ�
		state = END_STATE; // ���ӿ���
		setLED();
		printText();
		
		return;
	}
	
	feedCheck(); // ���̸� �Ծ����� Ȯ��

	for(i=MAX2_SIZE-1;i>0;i--) {
		course_x[i] = course_x[i-1];
		course_y[i] = course_y[i-1]; // ��θ� ������ ť�� ��ĭ�� �о
	}

	course_x[0] = snake.x;
	course_y[0] = snake.y; // ���ο� ��ġ�� ť�� ����

	matrix[course_y[0]][course_x[0]] = 2; // �Ӹ� ����

	for(i=1;i<snake.length;i++)
		matrix[course_y[i]][course_x[i]] = 1; // ������ ��� �� ���� ���̸�ŭ�� �������� �����.

	return;
}

void draw() { // Dotmatrix�� ���
	int i, j, ii, jj;
	int red, green;

	if(state==READY_STATE) { // ��� ���� ����
		for(ii=1;ii<256;ii<<=1) {
			COM_PORT = ii;
			RED_PORT = 0x00;
			GREEN_PORT = 0xFF; // ��� �ʷϻ�

			ms_delay(3);
		}
	}

	if(state==GAME_STATE) { // ���� ���� ����
		for(i=0,ii=1 ; i<MAX_SIZE ; i++,ii<<=1) {
			red = green = 0;
			COM_PORT = ii;

			for(j=0,jj=1 ; j<MAX_SIZE ; j++,jj<<=1) {
				switch(matrix[i][j]) {
				case 1:
					red += jj; // 1�̶��(���� ����) ������
					break;
				case 2:
					red += jj; // 2���(���� �Ӹ�) ��Ȳ��
				case 3:
					green += jj; // 3�̶��(����) �ʷϻ�
					break;
				default:
					break;
				}
	
				RED_PORT = red;
				GREEN_PORT = green;
			}

			ms_delay(3);
		}
	}

	if(state==END_STATE) { // ���� ����
		for(ii=1;ii<256;ii<<=1) {
			COM_PORT = ii;
			RED_PORT = 0xFF; // ��� ������
			GREEN_PORT = 0x00;

			ms_delay(3);
		}
	}

	return;
}





/****************************************
**************���� ���� Ȯ��**************
****************************************/

int collisionCheck() { // BOOL
	if(snake.x<0 || snake.x>=MAX_SIZE || snake.y<0 || snake.y>=MAX_SIZE) // ���� �� ������ ������ ��
		return TRUE; // ���ӿ���
	else if(matrix[snake.y][snake.x]==1) // ����� ����� ��
		return TRUE; // ���ӿ���
	else
		return FALSE;
}

int reverseCheck(int last_dir, int dir) { // BOOL
	if(last_dir==LEFT && dir==RIGHT) // ������ �̵������ ���� �Է¹��� �̵������� ���ݴ��϶�
		return TRUE; // ���ӿ���

	if(last_dir==RIGHT && dir==LEFT)
		return TRUE;
	
	if(last_dir==UP && dir==DOWN)
		return TRUE;
	
	if(last_dir==DOWN && dir==UP)
		return TRUE;

	return FALSE;
}

/****************************************/





void feedCheck() { // ���̸� �Ծ����� Ȯ��
	int bonus;

	bonus = 100 + rand() % 6; // ���� = 100 + ��(��, 0�¥��5)

	if(matrix[snake.y][snake.x]==3) { // ���̸� �Ծ��� ��
		++snake.length; // ��������
		score += bonus; // ����ȹ��

		if(speed>=20) // ���� �ӵ��� �ְ� �ƴ϶��
			speed -= 2; // �ӵ� ������ ���ҽ�Ų��. (���ҽ�ų���� ��������)

		locate(8, 1);
		dispString("        ");

		sprintf(score_str, "%d", score); // ������ ����� ������ ���ڿ��� ��ȯ

		locate(8, 1);
		dispString(score_str); // TextLCD�� ���

		is_feed = FALSE; // ���� Ȯ�� ������ �������� �ٲ۴�. (���� ����)
	}

	return;
}

void printText() { // ������ ���¿� ���� ���ڿ� ���
	locate(0, 0);
	dispString("                ");

	locate(0, 0);

	switch(state) {
	case READY_STATE:
		dispString("Press INT0 key! ");

		locate(0, 1);
		dispString("                ");

		break;

	case GAME_STATE:
		dispString("Game Playing..  ");

		locate(0, 1);
		dispString("Score : 0");

		break;

	case END_STATE:
		dispString("Game Over..     ");
		break;

	default:
		break;
	}

	return;
}

void us_delay(unsigned short time_us) {	// us���� ����
 	while (time_us>0) {
		asm volatile ("nop" ::);
	    asm volatile ("nop" ::);
	    asm volatile ("nop" ::);
	    asm volatile ("nop" ::);
	    asm volatile ("nop" ::);
	    asm volatile ("nop" ::);
	    asm volatile ("nop" ::);
	    asm volatile ("nop" ::);

	    --time_us;
	}

	return;
}

void ms_delay(unsigned short time_ms) { // ms���� ����
	register unsigned short i;

	for(i=0;i<time_ms;i++){	
		us_delay(250);	
		us_delay(250);	
		us_delay(250);	
		us_delay(250);	// 1us * 1000 = 1ms	
	}

	return;
}