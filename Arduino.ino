//create on 2016/09/23, ver. 1.0 by jason, edited on 2019/09/15, Mateusz Tchorek
#include <DFRobot_sim808.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <MechaQMC5883.h>
#define PHONE_NUMBER  "530093017"
DFRobot_SIM808 sim808(&Serial);
MechaQMC5883 qmc;
char buffer[380];
byte counter = 0;
byte left = 9;
byte middle = 10;
byte right = 11;
byte coordinatesMaxAmount = 10;
float coordinatesArray[10][2]; //latitude lo
char http_route[] = "GET /~mtchorek/route.php HTTP/1.0\r\n\r\n";
char http_get[] = "GET /~mtchorek/get.php HTTP/1.0\r\n\r\n";
char http_loc[] = "GET /~mtchorek/send.php?type=location&id=502261249&latlang=00.00000,00.00000 HTTP/1.0\r\n\r\n";
char http_del[] = "GET /~mtchorek/send.php?type=del&id=502261249&latlang=00.00000,00.00000 HTTP/1.0\r\n\r\n";

void initialize_qmc() {
  qmc.init(); //initialize compass library in case of electronic malfunctions
}

void clearOrInitializeCoordinatesArray(byte index = 99) {
  switch (index) {
    case 99:
      for (byte clearIndex = 0; clearIndex < coordinatesMaxAmount; clearIndex++) {
        coordinatesArray[clearIndex][0] = 0.0;
        coordinatesArray[clearIndex][1] = 0.0;
      }
      return;
    default:
      coordinatesArray[index][0] = 0.0;
      coordinatesArray[index][1] = 0.0;
      return;
  }
}

void fillLocationUrl(String latlang) {
  char * local;
  local = strstr (http_loc,"latlang=");
  
  for (byte b = 8;b<25;b++){
    local[b]=latlang[b-8];
  }
}
void fillDeleteUrl(String latlang) {
  char * local;
  local = strstr (http_del,"latlang=");
  
  for (byte b = 8;b<25;b++){
    local[b]=latlang[b-8];
  }
}

void connectToInternet() {
  while (!sim808.join(F("internet"))) {
    delay(2000);
  }
  while(true){
  if (sim808.connect(TCP,"sendzimir.metal.agh.edu.pl", 80)) {
    break;
  }
  delay(2000);
  }
}

void disconnectFromInternet() {
  sim808.close();
  sim808.disconnect();
  memset(buffer, 0, (short)sizeof(buffer));
}

// 50.0822292;latitude N/S 20.0264915;longitude W/E
bool routeRequest() {
  connectToInternet();
  delay(2000);
  sim808.send(http_route, sizeof(http_route) - 1);
  while (true) {
    short ret = sim808.recv(buffer, (short)sizeof(buffer) - 1);
    if (ret <= 0) {
      break;
    }
    buffer[ret] = '\0';
    break;
  }
  if (strstr(buffer, "true")) {
    disconnectFromInternet();
    return true;
  }
  else {
    disconnectFromInternet();
    return false;
  }
}

void getRoute() {
  connectToInternet();
  sim808.send(http_get, sizeof(http_get) - 1);
  while (true) {
    short ret = sim808.recv(buffer, (short)sizeof(buffer) - 1);
    if (ret <= 0) {
      break;
    }
    buffer[ret] = '\0';
    break;
  }
  char * pch;
  pch = strstr (buffer,"[[");
  byte arrayLength = getSize(pch);
  char coordinate[10];
  byte coordinateIndex=0;
  byte coordinatesIndex =0;
  for(byte index=2;index<arrayLength;index++){
      if (pch[index] == ','){
        coordinatesArray[coordinatesIndex][0]=atof(coordinate);
        memset(coordinate, 0, (byte)sizeof(coordinate));
        coordinateIndex =0;
        continue;  
      }
      if (pch[index] == ']'&&pch[index+1] == ','&&pch[index+2] == '['){
        coordinatesArray[coordinatesIndex][1]=atof(coordinate);
        index+=2;
        coordinatesIndex++;
        memset(coordinate, 0, (byte)sizeof(coordinate));
        coordinateIndex =0;
        continue;
      }
      if(pch[index]== ']' && pch[index+1] == ']'){
        coordinatesArray[coordinatesIndex][1]=atof(coordinate);
        continue;
      }
      coordinate[coordinateIndex]=pch[index];
      coordinateIndex++;
    }
    
  disconnectFromInternet();
}

byte getSize(char* ch){
      byte tmp=0;
      while (*ch) {
        *ch++;
        tmp++;
      }return tmp;}

void sendCoordinates(){
  connectToInternet(); 

  while(!sim808.init()) { 
      delay(1000);
      Serial.print("Sim808 init error\r\n");
  }
   
  sim808.attachGPS();
  while (true) {
    if (sim808.getGPS()) {
      break;
    }
  }
  float gpsLatitude = sim808.GPSdata.lat; // poziomo N/S
  float gpsLongitude = sim808.GPSdata.lon; //pionowo W/E
  sim808.detachGPS();

  String coordinates;
  coordinates+=String(gpsLatitude,5);
  coordinates+=F(",");
  coordinates+=String(gpsLongitude,5);

  fillLocationUrl(coordinates);

  sim808.send(http_loc, sizeof(http_loc) - 1);
  while (true) {
    short ret = sim808.recv(buffer, (short)sizeof(buffer) - 1);
    if (ret <= 0) {
      break;
    }
    buffer[ret] = '\0';
    break;
  }
  disconnectFromInternet();
}

void initializeOrGetRoute() {
  //request to server
  if (!routeRequest())
  {
    sim808.callUp(PHONE_NUMBER);
    sim808.callUp(PHONE_NUMBER);
    delay(120000);
    sim808.hangup();
    delay(100);
    sendCoordinates();
    delay(20000);
  }
  getRoute();
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  pinMode(left,OUTPUT); //9 LEFT
  pinMode(middle,OUTPUT); //10 MIDDLE 
  pinMode(right,OUTPUT);//11 RIGHT

  initialize_qmc();
  while (!sim808.init()) {
    delay(1000);
  }
  delay(3000);

  clearOrInitializeCoordinatesArray();
  initializeOrGetRoute();
}

void loop() {
  float userAzimuth, gpsLongitude, gpsLatitude, targetLongitude, targetLatitude, targetAzimuth;
  short x, y, z;
  targetLongitude = coordinatesArray[counter][1];
  targetLatitude = coordinatesArray[counter][0];
  unsigned short compassCounter = 0;
    
    while (coordinatesArray[counter % coordinatesMaxAmount][1] != 0.0 && coordinatesArray[counter % coordinatesMaxAmount][0] != 0.0 )
    {
        while(!sim808.init()) { 
            delay(1000);
            Serial.print("Sim808 init error\r\n");
        }
        sim808.attachGPS();
        while (true) {
           if (sim808.getGPS()) break;
        }
        Serial.print("glat: ");
        gpsLatitude = sim808.GPSdata.lat; // poziomo N/S
        Serial.print(gpsLatitude, 5);
        Serial.print(" glng: ");
        gpsLongitude = sim808.GPSdata.lon; //pionowo W/E
        Serial.println(gpsLongitude, 5);
        sim808.detachGPS();
        targetAzimuth = atan2(targetLongitude - gpsLongitude, targetLatitude - gpsLatitude) * ( 180.0 / M_PI );
    
        while (compassCounter < 65530 ) {
          if (compassCounter % 2000 == 0) {
            qmc.read(&x, &y, &z);
            userAzimuth = atan2(x, y) * ( 180.0 / M_PI );
    
            if (x == 0 && y == 0 && z == 0 || x == -1 && y == -1 && z == -1) {
              initialize_qmc();
            }
             if ( userAzimuth > -22.5 && userAzimuth <= 22.5) {
            Serial.println("N" );
          }
          if ( userAzimuth > 22.5 && userAzimuth <= 67.5) {
            Serial.println("NW ");
          }
          if ( userAzimuth > 67.5 && userAzimuth <= 112.5) {
            Serial.println("W ");
          }
          if ( userAzimuth > 112.5 && userAzimuth <= 157.5) {
            Serial.println("SW ");
          }
          if ( userAzimuth <= -157.5 || userAzimuth > 157.5) {
            Serial.println("S ");
          }
          if ( userAzimuth <= -112.5 && userAzimuth > -157.5) {
            Serial.println("SE ");
          }
          if ( userAzimuth <= -67.5 && userAzimuth > -112.5) {
            Serial.println("E ");
          }
          if ( userAzimuth <= -22.5 && userAzimuth > -67.5) {
            Serial.println("NE ");
          }
  
          Serial.print(targetAzimuth - userAzimuth);
          Serial.print(" ");
          Serial.print(targetAzimuth);
          Serial.print(" ");
          Serial.println(userAzimuth);
            
            if ( abs(targetAzimuth - userAzimuth) >= abs(targetAzimuth - userAzimuth - 360) && abs(targetAzimuth - userAzimuth + 360) >= abs(targetAzimuth - userAzimuth - 360)) {
                delay(100);
                digitalWrite(left, HIGH);
                digitalWrite(middle, LOW);
                digitalWrite(right, LOW);
            }
            if ( abs(targetAzimuth - userAzimuth) >= abs(targetAzimuth - userAzimuth + 360) && abs(targetAzimuth - userAzimuth - 360) >= abs(targetAzimuth - userAzimuth + 360) ) {
                delay(100);
                digitalWrite(left, LOW);
                digitalWrite(middle, LOW);
                digitalWrite(right, HIGH);
            }
            if ( abs(targetAzimuth - userAzimuth) <= abs(targetAzimuth - userAzimuth - 360) && abs(targetAzimuth - userAzimuth) <= abs(targetAzimuth - userAzimuth + 360) && targetAzimuth - userAzimuth > 0) {
                delay(100);
                digitalWrite(left, LOW);
                digitalWrite(middle, LOW);
                digitalWrite(right, HIGH);
            }
            if ( abs(targetAzimuth - userAzimuth) <= abs(targetAzimuth - userAzimuth - 360) && abs(targetAzimuth - userAzimuth) <= abs(targetAzimuth - userAzimuth + 360) && targetAzimuth - userAzimuth < 0) {
                delay(100);
                digitalWrite(left, HIGH);
                digitalWrite(middle, LOW);
                digitalWrite(right, LOW);
            }
          }
          compassCounter++;
        }
                delay(100);
                digitalWrite(left, LOW);
                digitalWrite(middle, LOW);
                digitalWrite(right, LOW);
        compassCounter = 0;
        if ( fabs(targetLongitude - gpsLongitude) < 0.00009 && fabs(targetLatitude - gpsLatitude) < 0.00009 ) {
          delay(100);
          digitalWrite(middle, HIGH);
          String coordinates;
          coordinates+=String(coordinatesArray[counter][0],5);
          coordinates+=F(",");
          coordinates+=String(coordinatesArray[counter][1],5);
          coordinatesArray[counter][1] = 0.0;
          coordinatesArray[counter][0] = 0.0;
          connectToInternet();
  
          fillDeleteUrl(coordinates);
          sim808.send(http_del, sizeof(http_del) - 1);
          
          while (true) {
            short ret = sim808.recv(buffer, (short)sizeof(buffer) - 1);
            if (ret <= 0) {
              break;
            }
            buffer[ret] = '\0';
            break;
          }
          disconnectFromInternet();
          counter++;
          if (counter == coordinatesMaxAmount) {
            counter = 0;
          }
          targetLongitude = coordinatesArray[counter][1];
          targetLatitude = coordinatesArray[counter][0];
        }
  }

  if (routeRequest())
  {
 // initializeOrGetRoute();
    getRoute();
    delay(50);
  }else {
    delay(100);
    while(true){
         digitalWrite(middle, HIGH);
    }
  }
}
