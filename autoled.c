#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <pthread.h> 

//DC motor
#define INA 12
#define INB 13

//Humidity
#define DHPIN 6
#define MAXTIME 100

//LED
#define LED 22

//Motion
#define TRIG 17
#define ECHO 18

//BUTTON
#define BTN_LED 23
#define BTN_MOTOR 24
#define BTN_SET 25

//Bluetooth
#define BAUD_RATE 115200

//Memory
int shared[6] = {FALSE, TRUE, TRUE, TRUE, 0, 50};

//Lock
pthread_mutex_t mutex =PTHREAD_MUTEX_INITIALIZER;


//function
void manageModule();
void *threadMotion();
void *threadHumidity();
void *threadBluetooth();
int Clock(int, int);
void initMotor();
void motor_Rotate(int, int);
int detectHumidity();
int detectMotion();
void connect();
void flush(int fd_serial);


//메인 쓰레드
int main(){
    if (wiringPiSetupGpio() == -1)
        return -1;

    pthread_t p_thread1; 
    pthread_t p_thread2;

    int th1, th2;
    
    th1 = pthread_create(&p_thread1, NULL, threadMotion, NULL);
    th2 = pthread_create(&p_thread2, NULL, threadHumidity,NULL);


    if(th1 < 0){
        perror("pthread_create() error\n");
        exit(0);
    }
    if(th2 < 0){ 
        perror("pthread_create() error\n");
        exit(0);
    }
    
    manageModule();
}

void manageModule(){
    pthread_t p_thread3;
    int th3;
    int setting=FALSE;


    pinMode(LED, OUTPUT);
    pinMode(BTN_LED, INPUT);
    pinMode(BTN_MOTOR, INPUT);
    pinMode(BTN_SET, INPUT);

    initMotor();

    int led_state=FALSE;
    int motor_state=FALSE;

    int led_output=FALSE;
    int motor_output=0;

    digitalWrite(LED, led_output);
    motor_Rotate(motor_output, 0);

    while(1){
        if(digitalRead(BTN_LED)){ 
            led_state = !led_state;
            if(led_state){
                shared[0] = TRUE;
                led_output = HIGH;
            }
            else{
                shared[0] = FALSE;
                led_output = LOW;
            }
            digitalWrite(LED, led_output);
        }

        if(digitalRead(BTN_MOTOR)){ 
            motor_state= !motor_state;
            if(motor_state){
                shared[0] = TRUE;
                shared[1] = TRUE;
                motor_output = 50;
            }
            else{
                shared[1] = FALSE;
                motor_output = 0;
            }
            motor_Rotate(motor_output, 0);
        }

        if(digitalRead(BTN_SET)){
            pthread_mutex_lock(&mutex);
            th3 = pthread_create(&p_thread3, NULL, threadBluetooth,NULL);
            if(th3 < 0){ 
                perror("pthread_create() error\n");
                exit(0);
            }
            pthread_join(p_thread3, NULL);
            pthread_mutex_unlock(&mutex);
        }

        delay(100);

        if(led_state && !shared[0]){
            led_state = FALSE;
            digitalWrite(LED, LOW);
        }

        if(motor_state && !shared[0] && !shared[1]){
            motor_state = FALSE;
            motor_Rotate(0, 0);
        }

        delay(100);
    }
}


//쓰레드 함수
void *threadMotion(){
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);

    int distance;
    int maximum = 50;

    while(1){
        if(shared[2]){
            pthread_mutex_lock(&mutex);
            distance = detectMotion();
            if (distance > maximum) {
                delay(1500);
                distance = detectMotion();
                if (distance > maximum) shared[0] = FALSE;
            }
            else shared[0] = TRUE;
            pthread_mutex_unlock(&mutex);
            delay(1500);
        }
    }
}

void *threadHumidity(){
    int humidity;
    while(1){
        if(shared[3]){
            pthread_mutex_lock(&mutex);
            humidity = detectHumidity();
            if(humidity < shared[5]) shared[1] = FALSE;
            shared[4]=humidity<100?humidity:shared[4];
            pthread_mutex_unlock(&mutex);
            delay(3000);
        }
    }
}

void *threadBluetooth(){
    connect();
}

//Bluetooth
unsigned char serialRead(const int fd)
{
    unsigned char x;
    if(read (fd, &x, 1) != 1) //read 함수를 통해 1바이트 읽어옴
    return -1;
    return x; //읽어온 데이터 반환
}

//1Byte 데이터를 송신하는 함수
void serialWrite(const int fd, const unsigned char c)
{
    write (fd, &c, 1); //write 함수를 통해 1바이트 씀
}

//입력버퍼 비움
void flush(int fd_serial){
    while(serialDataAvail (fd_serial)){
        serialRead(fd_serial);
    }
}

//스트링 출력
void printB(int fd_serial, char* string, int s_size){
    for(int i=0; i<s_size; i++){
       serialWrite(fd_serial, *(string+i));
    }
}

void connect() 
{
    static const char* UART2_DEV = "/dev/ttyAMA1"; //UART2 연결을 위한 장치 파일
    int fd_serial ; //UART2 파일 서술자
    unsigned char dat; //데이터 임시 저장 변수
    unsigned char op;
    char *menu;
    char *option;
    char *option2;

    if ((fd_serial = serialOpen (UART2_DEV, BAUD_RATE)) < 0){ //UART2 포트 오픈
        printf("Unable to open serial device.\n");
    }
    
    menu = "메뉴\n1.전등 절약모드 관리\n2.환풍기 절약모드 관리\n3.습도 단계 조절\n4.욕실 상태 확인\n";
    option = "1. On 2.Off";
    option2 = "적정습도 입력 (1~9단계):";


    printB(fd_serial, menu, strlen(menu));

    while(serialDataAvail(fd_serial)==0){}

    dat = serialRead (fd_serial); //버퍼에서 1바이트 값을 읽음

    flush(fd_serial);

    if(dat=='1'){
        printB(fd_serial, option, strlen(option));
        
        op = serialRead (fd_serial);
        flush(fd_serial);
            
        if(op=='1')
            shared[2]=TRUE;
        else
            shared[2]=FALSE;
    }
    else if(dat=='2'){         
        printB(fd_serial, option, strlen(option));

        op = serialRead (fd_serial);
        flush(fd_serial);
            
        if(op=='1')
            shared[3]=TRUE;
        else
            shared[3]=FALSE;
    }
    else if(dat=='3'){ 
        printB(fd_serial, option2, strlen(option2));
        
        op = serialRead(fd_serial);
        op-= 48;
        
        shared[5] = op*10;
    }
    else{
        char l_data[40]; 
        char h_data[40]; 
        
        char out[50] = "LED state:";
        sprintf(l_data, "%s\n", shared[0]?"ON":"OFF");
        strcat(out, l_data);

        strcat(out, "Humidity:");
        sprintf(h_data, "%d%%", shared[4]);
        strcat(out, h_data);

        printB(fd_serial, out, strlen(out));
    }
        
    flush(fd_serial);
    fflush (stdout) ;
    delay (10);
}


//DCMotor
int Clock(int range, int freq)
{ 
    return 19200000/(range*freq);
}

void initMotor()
{
    int duty = 0;
    pinMode(INA, PWM_OUTPUT); 
    pinMode(INB, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(100);
    pwmSetClock(Clock(100, 1000));

    pwmWrite(INA, duty);
    pwmWrite(INB, duty);
}
void motor_Rotate(int speed, int dir)
{
    if(dir==0){
        pwmWrite(INA, 0);
        pwmWrite(INB, speed);
    }
    else{
        pwmWrite(INB, 0);
        pwmWrite(INA, speed);
    }
}

//Humiditiy
int detectHumidity(){
    int dhtVal[5]={0,0,0,0,0};
    int laststate = HIGH;
    int counter = 0;
    int j=0;
    float f;
    dhtVal[0]=dhtVal[1]=dhtVal[2]=dhtVal[3]=dhtVal[4]=0;

    pinMode(DHPIN, OUTPUT);
    digitalWrite(DHPIN, LOW);
    delay(18);

    digitalWrite(DHPIN, HIGH);
    delayMicroseconds(40);

    pinMode(DHPIN, INPUT);

    for(int i=0; i<MAXTIME; i++){
        counter = 0;
        while(digitalRead(DHPIN)==laststate){
            counter++;
            delayMicroseconds(1);
            if(counter==255)
                break;
        }

        laststate = digitalRead(DHPIN);
        if(counter==255)
            break;
        if((i>=4) && (i%2==0)){
            dhtVal[j/8] <<= 1;
            if(counter>16){
                dhtVal[j/8] |= 1;
            }
            j++;
        }
    }

    if((j>=4) && (dhtVal[4]==((dhtVal[0]+dhtVal[1]+dhtVal[2]+dhtVal[3])&0xFF))){
        f=dhtVal[2]*9./5.+32;
        printf("Humidity=%d.%d%%Temperature=%d.%d*C(%0.1f*F)\n", dhtVal[0], dhtVal[1], dhtVal[2], dhtVal[3], f);
        return dhtVal[0];
    }
    else{
        printf("Invalid Data!\n");
        return 100;
    }
}

//Motion
int detectMotion(){

    digitalWrite(TRIG, FALSE);
    delay(1);

    int start;
    int end;

    float time;
    int distance;

    digitalWrite(TRIG, TRUE);
    delay(0.01);
    digitalWrite(TRIG, FALSE);

    //음파 왕복시간 측정
    while(digitalRead(ECHO)==0)
        start = clock();

    while(digitalRead(ECHO)==1)
        end = clock();

    time = (float)(end - start)/CLOCKS_PER_SEC;

    //거리계산
    distance = (int)(time * 343000 / 2); 
    printf("Distance: %d\n", distance);
    return distance;
}