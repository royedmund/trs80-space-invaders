/* MIDISD fast bridge v0.3 — classic Nano ATmega328P, 5V.
 * D10 TTL RX from MAX3232 at 9600; D1 hardware MIDI TX at 31250.
 * FD=25Hz, F9=30Hz, F8=tick boundary, FA=drain, FC=abort.
 * One-way, no checksum or flow control. NOT a general real-time MIDI bridge.
 * Complete tick groups are queued; short input delays rebuffer, not abort.
 * LED: slow blink playing, steady on fault, off idle.
 */
#include <SoftwareSerial.h>
SoftwareSerial trs(10,11);
constexpr uint16_t CAP=512, MASK=CAP-1;
constexpr uint32_t LEAD=160000UL, WATCHDOG=2000000UL;
uint8_t queue[CAP], held[16][16];
uint16_t head=0,tail=0,count=0,groups=0,peak=0,used=0;
uint32_t deadline=0,lastInput=0,period=40000;
uint16_t underruns=0; uint8_t frac=0,faultCode=0;
uint8_t status=0,first=0,pos=0;
bool sysex=false,terminateSysex=false,waiting=false,panic=false;
enum State:uint8_t{RAW,STREAM,DRAIN,FAULT};State state=RAW;
uint8_t panicCh=0,panicCC=0,panicByte=0,panicStatus=0,panicData=0;
uint16_t panicNote=0;
static bool due(uint32_t a,uint32_t b){return int32_t(a-b)>=0;}
static void clearQueue(){head=tail=count=groups=0;waiting=false;}
static void requestPanic(){clearQueue();terminateSysex|=sysex;status=pos=0;panic=true;panicCh=panicCC=panicByte=0;panicNote=0;}
static void fail(uint8_t why){if(state==FAULT)return;faultCode=why;requestPanic();state=FAULT;digitalWrite(LED_BUILTIN,HIGH);}
static void start(uint8_t b,uint32_t now){
  if(state!=RAW||count||used||sysex||panic)requestPanic();else clearQueue();
  period=b==0xfd?40000UL:33333UL;frac=0;peak=underruns=0;
  deadline=now+LEAD;lastInput=now;state=STREAM;faultCode=0;
}
static void put(uint8_t b){
  if(count==CAP){fail(1);return;}
  queue[head]=b;head=(head+1)&MASK;++count;if(count>peak)peak=count;
}
static void receive(uint8_t b,uint32_t now){
  if(b==0xfd||b==0xf9){start(b,now);return;}
  if(b==0xfc){if(state!=FAULT){requestPanic();state=RAW;}return;}
  if(state==FAULT||state==DRAIN)return;
  // Checked v0.2 START is incompatible: fail visibly, not as raw music.
  if(b==0xf4||b==0xf5){fail(4);return;}
  if(state==STREAM){
    lastInput=now;put(b);if(state==FAULT)return;
    if(b==0xf8||b==0xfa)++groups;
    if(b==0xfa)state=DRAIN;
  }else if(b!=0xf8&&b!=0xfa)put(b);
}
// Track only MIDI messages actually accepted by hardware TX. Running status
// supported; note-on velocity zero is note-off. No per-byte timestamps.
static void sendMusic(uint8_t b){
  Serial.write(b);
  if(b>=0xf8)return;
  if(b&128){
    if(b==0xf0){sysex=true;status=pos=0;return;}
    sysex=false;pos=0;if(b>=0xf0){status=0;return;}
    status=b;used|=uint16_t(1)<<(b&15);return;
  }
  if(!status||sysex)return;
  if((status&0xe0)==0xc0)return;
  if(!pos){first=b;pos=1;return;}pos=0;
  uint8_t type=status&0xf0;if(type!=0x80&&type!=0x90)return;
  uint8_t &v=held[status&15][first>>3],mask=uint8_t(1<<(first&7));
  if(type==0x90&&b)v|=mask;else v&=uint8_t(~mask);
}
// Only channels used by the stream need cleanup (plus channel 1).
// Send complete messages incrementally, never block on a full UART TX ring.
static void servicePanic(){
  if(terminateSysex){Serial.write(0xf7);sysex=terminateSysex=false;return;}
  if(panicByte){Serial.write(panicByte==1?panicData:0);if(++panicByte==3)panicByte=0;return;}
  while(panicCh<16 && !((used|1)&(uint16_t(1)<<panicCh)))++panicCh;
  if(panicCh==16){panic=false;used=0;status=pos=0;return;}
  if(panicCC<3){static const uint8_t cc[]={64,66,67};panicStatus=0xb0+panicCh;panicData=cc[panicCC++];}
  else if(panicCC==3){
    while(panicNote<128 && !(held[panicCh][panicNote>>3]&(1<<(panicNote&7))))++panicNote;
    if(panicNote==128){++panicCC;return;}
    panicStatus=0x80+panicCh;panicData=panicNote++;
  }else if(panicCC==4){panicStatus=0xb0+panicCh;panicData=123;++panicCC;}
  else{for(uint8_t i=0;i<16;++i)held[panicCh][i]=0;++panicCh;panicCC=0;panicNote=0;return;}
  Serial.write(panicStatus);panicByte=1;
}
static void advanceTick(){
  deadline+=period;
  if(period==33333UL && ++frac==3){++deadline;frac=0;}
  // Tiny occupancy-based slew compensates for independent clocks without
  // reacting to jitter on individual serial bytes. Nominal lead is 4/5 ticks.
  uint8_t high=period==40000UL?5:6;
  if(groups>high)deadline-=200;
  else if(groups<2)deadline+=200;
}
static void service(uint32_t now){
  if(Serial.availableForWrite()<56)return; // small HW queue; responsive STOP
  if(panic||terminateSysex){servicePanic();return;}
  if(state==FAULT)return;
  if(state!=RAW){
    if(!due(now,deadline))return;
    if(!groups){if(!waiting){waiting=true;++underruns;}return;}
    // A temporarily late sender is not a link failure. Resume from here
    // rather than dumping a backlog of overdue notes at once.
    if(waiting){deadline=now;waiting=false;}
  }
  if(!count)return;
  uint8_t b=queue[tail];tail=(tail+1)&MASK;--count;
  if(state!=RAW&&b==0xf8){--groups;advanceTick();return;}
  if(state!=RAW&&b==0xfa){--groups;requestPanic();state=RAW;return;}
  sendMusic(b);
}
void setup(){pinMode(LED_BUILTIN,OUTPUT);digitalWrite(LED_BUILTIN,LOW);Serial.begin(31250,SERIAL_8N1);trs.begin(9600);}
void loop(){
  if(trs.overflow())fail(2);
  // Drain RX first; output is interleaved to avoid starving either UART.
  for(uint8_t i=0;i<16&&trs.available();++i){int b=trs.read();if(b>=0)receive(uint8_t(b),micros());service(micros());}
  uint32_t now=micros();
  if(state==STREAM&&due(now,lastInput+WATCHDOG))fail(3);
  for(uint8_t i=0;i<8;++i)service(now);
  // LED never shares MIDI TX; no diagnostic strings on either UART.
  static uint8_t old=255;uint8_t level=state==FAULT?HIGH:((state==STREAM||state==DRAIN)?((now>>18)&1):LOW);
  if(level!=old){digitalWrite(LED_BUILTIN,level);old=level;}
}
