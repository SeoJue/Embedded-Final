# 2022-임베디드시스템 최종과제 
# 화장실 전등관리 시스템
#### Project name : 까먹 등? 꺼졌 등!
#### Team name : LNI
#### Project execution period : 2022.12.01~2022.12.15
-----------------------

# 1. 개요

## 개발배경
일상생활을 하다보면 욕실을 사용후 전등을 끄고 나오지 않는 경우가 빈번하게 발생한다. 
한참이 지난 후 욕실에 다시 들어갔을 때 몇십분~몇시간 동안 켜져있었을 전등을 보면 기분도 찝찝하고, 지속되면 훗날 높은 전기요금 고지서를 받게될 수도 있다.


## 필요성 
이를 해결하는 가장 좋은 방법은 전등을 끄는 습관을 들이거나 잘 기억하는 것이겠지만
바쁜 현대인들에게는 이런 사소한 것까지 신경 쓸 여유는 없다. 
iot 관련 기술과 스마트홈이 발전하고 있는 지금 관련 시스템에 화장실 전등을 관리하는 시스템이 하나 들어가는 것도 괜찮아보인다.  
 
  
# 2. 개발내용

## 최종목표
사용감을 위해 기존의 스위치를 통한 전등, 환풍기의 on off 기능은 유지하되
욕실 내부에 사람이 없거나 습도가 충분히 낮아지는 등 해당 기기가 더이상 동작할 이유가 없어진다면 
사용자가 전원을 끄는 것을 잊어버리더라도 시스템 자체적으로 동작을 멈출 수 있는 시스템을 구축한다.

## 주요기능
> ### 전등 관리   
욕실 내부에 사람이 없다면 자동으로 전등을 끈다.

> ### 환풍기 관리   
욕실 내부에 사람이 없고, 습도도 일정 수치 이하로 낮아졌다면 자동으로 환풍기를 끈다.

> ### 시스템 관리  
컨트롤러를 이용하여 해당 시스템을 관리한다. (관리 시스템의 on off, 욕실 내부 상태 정보 확인 등) 

  
# 3. 시스템 구성

### \<시스템 구조\>  
![외부](https://user-images.githubusercontent.com/69377952/206743356-79508ffe-ea2d-4b4f-894f-20165511cc44.png)
![내부](https://user-images.githubusercontent.com/69377952/206747651-c84a4d5e-1f55-4624-8d13-3a7d5d9e7035.png)

### \<시스템 흐름\>  
![흐름1](https://user-images.githubusercontent.com/69377952/206745147-742881d0-eb55-48aa-9fe9-f46af03d40a9.png)  
![흐름2](https://user-images.githubusercontent.com/69377952/206745222-6cd8cd67-f424-441c-b48e-62609bad163d.png)  
![흐름3](https://user-images.githubusercontent.com/69377952/206745281-3fe1d79f-369b-4636-85ca-b8c090fb35fe.png)  
![흐름4](https://user-images.githubusercontent.com/69377952/206745327-28e15edf-7660-4b16-959f-51f8cf7afa88.png)  


# 4. 개발내용 
### \<하드웨어\>  
![물리](https://user-images.githubusercontent.com/69377952/207568285-4fde78c9-0515-4c5e-9675-499637b579bd.jpg)


### \<공유 메모리\>  
``` C
int shared[6] = {FALSE, TRUE, TRUE, TRUE, 0, 50};
```
> 쓰레드 간의 데이터 공유를 위해 전역 배열  
각각의 인덱스가 저장하는 정보  
0번: 욕실 내부 사람 여부  
1번: 적정 습도 도달 여부  
2번: 욕실 내부 사람 탐지 기능 동작 여부  
3번: 욕실 내부 습도 측정 기능 동작 여부  
4번: 욕실 내부 습도 수치  
5번: 욕실 적정 습도 수치 

### \<거리 감지 쓰레드\>
```C
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
```
> 지속적으로 근처에 사람이 있는지 탐지하는 쓰레드  
거리 센서와의 거리가 일정 수치 이상이면 사람이 없다고 판단 

### \<거리 감지 함수\>
```C
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
```
> 거리감지 센서를 이용하여 센서로부터 가장 가까운 사물까지의 거리를 반환하는 함수 

### \<습도 감지 쓰레드\>
```C
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
```
> 지속적으로 주변습도를 기록하고 습도가 적정한지 판단하는 쓰레드  

### \<습도 감지 함수\>
```C
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
```
> 온습도 센서를 이용하여 주변 습도를 감지하고 반환하는 함수 


### \<메인 쓰레드\>
```C
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
```
> 시스템을 시작하고 LED와 모터의 직접적인 제어를 담당하는 중심 쓰레드  
습도 감지 쓰레드, 거리 감지 쓰레드, 블루투스 쓰레드 등을 생성



### \<버튼 제어 함수 \>
```C
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
```
> 메인 쓰레드에서 실행되는 함수  
버튼 입력에 따라 LED와 모터를 ON, OFF하는 기능, 블루투스 통신 쓰레드 생성하는 기능 수행  
공유 데이터를 읽어 LED와 모터의 자동 ON, OFF 기능 수행  


### \<모터 제어 함수 \>
```C
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
```
> 모터 설정을 초기화 하는 함수
> 파라미터에 따라 모터 속도와 방향을 조절하는 함수 


### \<블루투스 실행 쓰레드 \>
```C
void *threadBluetooth(){
    connect();
}
```
> 블루투스 통신을 실행하는 쓰레드


### \<블루투스 통신 함수\>
```C
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
```
> 연결 기기와 통신하여 시스템 설정과 데이터 출력 등을 수행하는 함수 

# 5. 요구사항
### \<쓰레드 이용\>
`threadMotion()`  
`threadHumidity()`  
`threadBluetooth()`  

> 외부 데이터 감지와, 블루투스 통신을 위해 메인 쓰레드 외에 3개의 추가적 쓰레드 생성 

### \<쓰레드 간 통신\>
int shared[6] = {FALSE, TRUE, TRUE, TRUE, 0, 50};

>전역 변수를 이용하여 쓰레드 간 데이터를 공유 

### \<LOCK 사용\>
```C
pthread_mutex_lock(&mutex);
distance = detectMotion();
if(distance > maximum){
  delay(1500);
  distance = detectMotion();
  if(distance > maximum) shared[0] = FALSE;
}
pthread_mutex_unlock(&mutex);
```
> threadMotion() 내부 


```C
pthread_mutex_lock(&mutex);
humidity = detectHumidity();
if(humidity < shared[5]) shared[1] = FALSE;
shared[4]=humidity<100?humidity:shared[4];
pthread_mutex_unlock(&mutex);
```
> threadHumidity() 내부 


```C
pthread_mutex_lock(&mutex);
th3 = pthread_create(&p_thread3, NULL, threadBluetooth,NULL);
if(th3 < 0){ 
  perror("pthread_create() error\n");
  exit(0);
}
pthread_join(p_thread3, NULL);
pthread_mutex_unlock(&mutex);
```
> mnageModule() 내부 


>메인 쓰레드를 제외한 다른 쓰레드들이 
>자신의 기능을 수행하고 공유 메모리를 접근하는 부분에서 Mutex Lock 사용

### \<외부통신\>
`void connect() `  
`threadBluetooth()`

>UART 통신을 통해 블루투스로 외부와 통신 구현 


# 6. 문제점 및 해결방안

## 문제점
### 1. 온습도 센서 문제  
> 무슨 이유에서인지 온습도 센서는 멀티 쓰레드 환경에서  
> 다른 쓰레드와 동시에 수행될 때 정상적인 데이터를 읽어오지 못하는 문제 발생 

### 2. 블루투스 통신 문제 
> 블루투스 기기를 이용해 데이터를 입력 받을 때 
> 하나의 문자열만을 전송 했는데도 여러개의 데이터가 읽히는 문제 발생

## 해결방안
### 1. 온습도 센서 문제 해결
> 온습도 센서의 정상적 수행을 위해 메인 쓰레드를 제외한 다른 쓰레드들이  
> 동시에 자신의 기능 함수를 수행할 수 없도록 LOCK을 추가하여 문제를 해결했다. 

### 2. 블루투스 통신 문제 해결
> 입력 버퍼를 비우는  `flush()` 함수를 구현하고  
> 외부 기기로 부터 데이터를 전송 받는 동작 이후에는  
> 항상 flush()함수를 수행하도록 하여 입력 문제를 해결하였다.


# 7. 사용 설명서 
![설명1](https://user-images.githubusercontent.com/69377952/207569535-1bfb1f3c-6245-4b7c-856e-1de70113d274.jpg)

### \<시스템 버튼\>
> 시스템 버튼은 다음과 같다.  
> LED 버튼과 모터버튼을 누르면 해당 기기의 작동 상태가 ON-OFF로 전환된다.  
> 제어 버튼을 누르면 블루투스가 연결된 핸드폰 어플로 시스템 설정이 가능하다.  

![설명2](https://user-images.githubusercontent.com/69377952/207569581-00ab86b1-c386-41fa-8c28-b4ba8e075bf8.jpg)

### \<기기 동작\>
> 해당 버튼을 누를 경우 이미지와 같이 기기가 동작한다. 별도의 시스템 설정을 하지 않았다면  
> 전등관리와 환풍기관리가 모두 활성화된 상태이며 거리센서가 거리 내 물체를 탐지하지 못하면 LED가 자동으로 OFF 되고  
> 동시에 내부 습도가 적정 습도 이하라면 모터도 자동으로 OFF 된다. (단 적정습도 이하라도 내부에 사람이 있으면 자동으로 OFF되지 않음)

![설정1](https://user-images.githubusercontent.com/69377952/207571184-8e59b515-6af5-4b90-a40a-32d14164ce92.jpg)

### \<설정 메뉴\>
> 제어 버튼을 누르면 핸드폰 어플에는 위와 같은 문구가 출력된다.  
> 1: LED의 자동관리 기능을 ON, OFF하는 옵션.  
> 2: 모터의 자동관리 기능을 ON, OFF하는 옵션.  
> 3: 모터가 꺼지는 욕실 내부 적정 습도를 설정하는 옵션.
> 4: 현재 욕실 내부의 사람 여부와 습도 수치를 출력하는 옵션.

![설정2](https://user-images.githubusercontent.com/69377952/207571240-929d6918-34fb-4be3-aae0-5b50dced5db0.jpg)

### \<자동관리 옵션\>
> LED와 모터의 자동관리 옵션을 선택하면 출력되는 문구이다.   
> 1을 입력하면 관리 기능을 키고, 2를 입력하면 관리 기능을 끈다.  

![설정3](https://user-images.githubusercontent.com/69377952/207571285-79adc272-5653-45ff-8d08-07424e20e534.jpg)

### \<적정습도 설정 옵션\>
> 습도 설정 옵션을 선택하면 출력되는 문구이다.  
> 습도는 단계로 나타낼 수 있으며 1단계: 10% ~ 9단계: 90%로 설정 가능하다. 

![설정4](https://user-images.githubusercontent.com/69377952/207571312-0010d9f1-0d86-48c0-be49-04ef785ebc05.jpg)

### \<적정습도 설정 옵션\>
> 시스템 정보를 선택하면 출력되는 문구이다.  
> 내부 사람 여부와 습도 수치를 확인할 수 있으며  
> 이미지에서는 사람이 없어 OUT, 최초 습도 측정이 되지 않아 0%로 출력하는 상태이다.


# 8. 조건

## 환경
> C (Linux)  
> Raspberry Pi 4 (Linux Ubunto)

## 라이브러리
`<stdio.h>`  

`<stdlib.h>`  

`<unistd.h>`  

`<string.h>`  

`<wiringPi.h>`  

`<wiringSerial.h>`  

`<pthread.h>` 

