#include <avr/io.h>
#include <avr/interrupt.h> // 인터럽트를 사용하기 위해
#include <stdio.h> // for sprintf()
#include <stdlib.h> // for srand()

#define LEFT 75 // 이동방향
#define RIGHT 77
#define UP 72
#define DOWN 80

#define MAX_SIZE 8 // Dotmatrix의 크기
#define MAX2_SIZE 64 // Dotmatrix^2의 크기

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

enum BOOL{FALSE, TRUE}; // bool 타입을 지원하지 않아서 enum을 이용해 임시로 사용
enum STATE{READY_STATE, GAME_STATE, END_STATE}; // 게임의 상태

typedef struct SNAKE {
	int x;
	int y;
	int length;
	int dir;
}SNAKE; // 뱀 구조체





/****************************************
**************전역 변수 선언**************
****************************************/

int cnt = 0; // for OVERFLOW0
int led = 0; // for LED
int speed = 50; // 성장에 따라 게임의 속도를 빨라지게 하기위한 변수

int state, score, last_dir;
int is_feed; // BOOL
SNAKE snake;
char score_str[16];
int course_x[MAX2_SIZE], course_y[MAX2_SIZE]; // 지나온 경로를 저장한다.
int matrix[MAX_SIZE][MAX_SIZE];

/****************************************/





/****************************************
**********함수의 프로토타입 선언**********
****************************************/

SIGNAL(SIG_OVERFLOW0); // 인터럽트
SIGNAL(SIG_INTERRUPT0);
SIGNAL(SIG_INTERRUPT3);
SIGNAL(SIG_INTERRUPT7);
SIGNAL(SIG_INTERRUPT4);
SIGNAL(SIG_INTERRUPT5);

void initTimer(); // 타이머

void enablePulse(); // TextLCD
void initLCD();
void sendLCDcommand(unsigned char);
void sendLCDdata(unsigned char);
void dispString(char*);
void locate(int, int);

void setLED(); // LED

void snakeInit(); // 게임 내에서 필요한 함수
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
	// 각 포트의 입출력과 초기값 설정
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
	// time.h가 없어서 time() 함수를 쓸 수 없었다.
	// 대신, time() 함수처럼 계속해서 값이 변경되는 TCNT0을 사용했다.

	sei(); // 전체 인터럽트 허가
	initLCD();

	state = READY_STATE;
	printText();

	while(1) { // 계속 반복한다.
		switch(state) { // 현재 상태에 따라 동작한다.
		case READY_STATE: 
			draw(); // 반복해서 빠르게 그린다.
			break;

		case GAME_STATE:
			snakeInit(); // 뱀 초기화
			matrixInit(); // 맵 초기화
			courseInit(); // 경로를 저장할 큐 초기화

			is_feed = FALSE; // 초기값으로 먹이가 없음을 설정해야 먹이생성 함수에 의해 먹이가 임의의 위치에 생성된다.
			score = 0; // 초기 점수
			speed = 50; // 초기 속도, 값이 작아질수록 빨라진다. 자세한 내용은 SIGNAL(SIG_OVERFLOW0)을 참고.
			
			while(state==GAME_STATE)
				draw(); // 반복해서 빠르게 그린다.

			break;

		case END_STATE:	
			draw(); // 반복해서 빠르게 그린다.
			break;

		default:
			break;
		}
	}

	return 0;
}

SIGNAL(SIG_OVERFLOW0) {
	cli();

	TCNT0 = 112; // 0.01초마다 인터럽트 발생
	++cnt;
	
	if(cnt==speed) { // cnt가 speed와 같아질 때마다 실행한다. speed의 값은 뱀이 먹이를 먹을 때마다 감소하므로, 결국 게임은 점점 빨라지게 된다.
		cnt = 0;

		if(led==128) // LED 출력을 위한 변수 설정
			led = 1;
		else
			led <<= 1;
		
		if(state==GAME_STATE) {
			move(); // 뱀을 움직이고
			setLED(); // LED도 변화시킨다.

			if(!is_feed) // 먹이가 없으면
				makeFeed(); // 먹이를 만든다.
		}
	}

	sei();
}

SIGNAL(SIG_INTERRUPT0) { // ENTER
	switch(state) {
	case READY_STATE: // 준비상태일때
		state = GAME_STATE; // 게임상태로 바꾼다.
		setLED();
		printText();
		break;

	case GAME_STATE: // 게임 중일때는 변화없음
		break;

	case END_STATE: // 종료상태일때
		state = READY_STATE; // 준비상태로 바꾼다.
		setLED();
		printText();
		break;
		
	default:
		break;
	}
}

SIGNAL(SIG_INTERRUPT3) { // LEFT
	if(state==GAME_STATE) { // 게임 중일때만 작동한다.
		last_dir = snake.dir; // 이전 뱀의 이동 방향을 변수에 저장하고
		snake.dir = LEFT; // 뱀의 이동 방향을 입력한 키에 맞게 변화시킨다.
	
		if(snake.length!=1 && reverseCheck(last_dir, snake.dir)) { // 뱀의 길이가 2이상이고, 기존의 이동방향과 반대되는 방향의 키를 눌렀다면
			state = END_STATE; // 게임오버
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

void initTimer() { // 타이머, 카운터 동작
	TCCR0 =	0x07;	
	TCNT0 =	112;
	TIMSK =	0x01;	
	TIFR = 0x00;

	return;
}





/****************************************
************TextLCD 관련 함수************
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





void setLED() { // 게임의 상태에 따라 LED출력
	switch(state) {
	case READY_STATE: // 준비 상태일때
		LED1_PORT = 0x00;
		LED2_PORT = 0x00; // 모두 꺼진다.

		break;

	case GAME_STATE: // 게임 중일때
		switch(led) { // led 변수 값에 맞게 출력한다.
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

	case END_STATE: // 종료 상태일때
		LED1_PORT = 0x0F;
		LED2_PORT = 0xF0; // 연결된 LED를 모두 켠다.

		break;

	default:
		break;
	}

	return;
}

void snakeInit() { // 뱀 초기화
	snake.x = snake.y = 0; // 뱀의 초기 위치는 (0, 0)
	snake.length = 1; // 뱀의 초기 길이는 1
	snake.dir = RIGHT; // 뱀의 초기 방향은 오른쪽

	return;
}

void matrixInit() { // 맵 초기화
	int i, j;

	for(i=0;i<MAX_SIZE;i++) {
		for(j=0;j<MAX_SIZE;j++) {
			if(i==0&&j==0)
				matrix[i][j] = 2; // 2는 뱀의 머리를 나타낸다.
			else
				matrix[i][j] = 0; // 0은 아무것도 없음을 나타낸다.
		}
	}

	return;
}

void courseInit() { // 경로 초기화
	int i;

	course_x[0] = snake.x;
	course_y[0] = snake.y; // 큐에 뱀의 초기 위치를 넣는다.

	for(i=1;i<MAX2_SIZE;i++)
		course_x[i] = course_y[i] = -1; // 나머지는 -1로 초기화

	return;
}

void makeFeed() { // 먹이 생성
	int x, y;

	while(1) {
		x = rand() % 8 + 1;
		y = rand() % 8 + 1; // 임의의 x, y값을 생성한다.
			
		if(matrix[y][x]==0) { // (x, y) 위치에 아무것도 없다면
			matrix[y][x] = 3; // (x, y) 위치에 먹이를 생성한다. 3은 먹이를 의미한다.
			break;
		}
	}

	is_feed = TRUE; // 먹이 확인 변수를 참으로 바꾼다. (먹이 있음)

	return;
}

void move() { // 이동
	register int i;

	for(i=0;i<snake.length;i++) {
		if(i==snake.length-1)
			matrix[course_y[i]][course_x[i]] = 0; // 뱀의 꼬리부분은 0으로
		else
			matrix[course_y[i]][course_x[i]] = 1; // 나머지는 1으로 바꾼다.
	}

	// 그 이유는 뱀의 머리가 2, 몸통이 1, 그 외의 빈 공간을 0이라 했을 때, 
	// 000000000211111111111000000000
	// 000000002111111111110000000000
	// 과 같이 변하기 때문이다. (뱀이 이동함에 따라 꼬리가 있던 곳이 0으로 바뀜)
	// (머리부분은 함수의 아래쪽에서 따로 만든다.)

	switch(snake.dir) { // 뱀의 이동 방향에 따라 머리의 위치를 변화시킨다.
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

	if(collisionCheck()) { // 벽이나 자신의 몸통과 충돌했다면
		state = END_STATE; // 게임오버
		setLED();
		printText();
		
		return;
	}
	
	feedCheck(); // 먹이를 먹었는지 확인

	for(i=MAX2_SIZE-1;i>0;i--) {
		course_x[i] = course_x[i-1];
		course_y[i] = course_y[i-1]; // 경로를 저장한 큐를 한칸씩 밀어냄
	}

	course_x[0] = snake.x;
	course_y[0] = snake.y; // 새로운 위치를 큐에 넣음

	matrix[course_y[0]][course_x[0]] = 2; // 머리 생성

	for(i=1;i<snake.length;i++)
		matrix[course_y[i]][course_x[i]] = 1; // 지나온 경로 중 뱀의 길이만큼을 몸통으로 만든다.

	return;
}

void draw() { // Dotmatrix에 출력
	int i, j, ii, jj;
	int red, green;

	if(state==READY_STATE) { // 대기 중인 상태
		for(ii=1;ii<256;ii<<=1) {
			COM_PORT = ii;
			RED_PORT = 0x00;
			GREEN_PORT = 0xFF; // 모두 초록색

			ms_delay(3);
		}
	}

	if(state==GAME_STATE) { // 게임 중인 상태
		for(i=0,ii=1 ; i<MAX_SIZE ; i++,ii<<=1) {
			red = green = 0;
			COM_PORT = ii;

			for(j=0,jj=1 ; j<MAX_SIZE ; j++,jj<<=1) {
				switch(matrix[i][j]) {
				case 1:
					red += jj; // 1이라면(뱀의 몸통) 빨간색
					break;
				case 2:
					red += jj; // 2라면(뱀의 머리) 주황색
				case 3:
					green += jj; // 3이라면(먹이) 초록색
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

	if(state==END_STATE) { // 게임 오버
		for(ii=1;ii<256;ii<<=1) {
			COM_PORT = ii;
			RED_PORT = 0xFF; // 모두 빨간색
			GREEN_PORT = 0x00;

			ms_delay(3);
		}
	}

	return;
}





/****************************************
**************종료 조건 확인**************
****************************************/

int collisionCheck() { // BOOL
	if(snake.x<0 || snake.x>=MAX_SIZE || snake.y<0 || snake.y>=MAX_SIZE) // 뱀이 맵 밖으로 나갔을 때
		return TRUE; // 게임오버
	else if(matrix[snake.y][snake.x]==1) // 몸통과 닿았을 때
		return TRUE; // 게임오버
	else
		return FALSE;
}

int reverseCheck(int last_dir, int dir) { // BOOL
	if(last_dir==LEFT && dir==RIGHT) // 이전의 이동방향과 새로 입력받은 이동방향이 정반대일때
		return TRUE; // 게임오버

	if(last_dir==RIGHT && dir==LEFT)
		return TRUE;
	
	if(last_dir==UP && dir==DOWN)
		return TRUE;
	
	if(last_dir==DOWN && dir==UP)
		return TRUE;

	return FALSE;
}

/****************************************/





void feedCheck() { // 먹이를 먹었는지 확인
	int bonus;

	bonus = 100 + rand() % 6; // 점수 = 100 + α(단, 0≤α≤5)

	if(matrix[snake.y][snake.x]==3) { // 먹이를 먹었을 때
		++snake.length; // 길이증가
		score += bonus; // 점수획득

		if(speed>=20) // 현재 속도가 최고가 아니라면
			speed -= 2; // 속도 변수를 감소시킨다. (감소시킬수록 빨라진다)

		locate(8, 1);
		dispString("        ");

		sprintf(score_str, "%d", score); // 정수로 저장된 점수를 문자열로 변환

		locate(8, 1);
		dispString(score_str); // TextLCD에 출력

		is_feed = FALSE; // 먹이 확인 변수를 거짓으로 바꾼다. (먹이 없음)
	}

	return;
}

void printText() { // 게임의 상태에 따라 문자열 출력
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

void us_delay(unsigned short time_us) {	// us단위 지연
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

void ms_delay(unsigned short time_ms) { // ms단위 지연
	register unsigned short i;

	for(i=0;i<time_ms;i++){	
		us_delay(250);	
		us_delay(250);	
		us_delay(250);	
		us_delay(250);	// 1us * 1000 = 1ms	
	}

	return;
}