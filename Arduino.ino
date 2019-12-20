//create on 2016/09/23, ver. 1.0 by jason, edited on 2019/09/15, Mateusz Tchorek
#include <DFRobot_sim808.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <MechaQMC5883.h>
#define PHONE_NUMBER  "530093017"   
DFRobot_SIM808 sim808(&Serial);
MechaQMC5883 qmc;

char buffer[650];
byte counter = 0;
byte coordinatesMaxAmount = 20;
float coordinatesArray[20][2];
void initialize_qmc(){
  qmc.init(); //initialize compass library in case of electronic malfunctions
}
char url[] = "sendzimir.metal.agh.edu.pl";
char http_route[] = "GET /~mtchorek/route.php HTTP/1.0\r\n\r\n";
char http_get[] = "GET /~mtchorek/get.php HTTP/1.0\r\n\r\n";
char http_loc[94] = "GET /~mtchorek/send.php?type=location&id=502261249&latlang=";


char http_suffix[] = " HTTP/1.0\r\n\r\n";
void clearOrInitializeCoordinatesArray(byte index=99){
  switch(index){
    case 99:
    for(byte clearIndex = 0; clearIndex<coordinatesMaxAmount;clearIndex++){
      coordinatesArray[clearIndex][0]=0.0;
      coordinatesArray[clearIndex][1]=0.0;
    }
    return;
    default:
    coordinatesArray[index][0]=0.0;
    coordinatesArray[index][1]=0.0;
    return;
  }
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  initialize_qmc();
  while(!sim808.init()) { delay(1000);}
  if( sim808.attachGPS()){Serial.println("OGPS success");}
  else Serial.println("OGPS fail");//to remove
    while(!sim808.join(F("internet"))) {
      Serial.println("Sim808 join network error");
      delay(2000);
  }
  Serial.print("IP Address is ");
  Serial.println(sim808.getIPAddress());
  clearOrInitializeCoordinatesArray();
  initializeRoute();
}
// 50.0822292;latitude N/S 20.0264915;longitude W/E
bool routeRequest(){

}

void initializeRoute(){
  //request to server
  routeRequest();
  
  // if response.isTravel == false or response.coordinates.size = 0
  //  sim808.callUp(SERVICE_NUMBER);
  // wait for route calculating
//   for (byte routeIndex = 0; routeIndex < response.coordinatesSize ; routeIndex++){
//   coordinatesArray[routeIndex][0] = response[routeIndex].getLatitude();
//   coordinatesArray[routeIndex][1] = response[routeIndex].getLongnitude();
//   }
// load 
  Serial.println("Initializing user route");
}

void loop() {
    float userDeclination, gpsLongitude, gpsLatitude, targetLongitude, targetLatitude, targetDeclination;
    short x, y, z;
    targetLongitude = coordinatesArray[counter][1];
    targetLatitude = coordinatesArray[counter][0];

    unsigned short compassCounter = 0;
    initializeRoute();
    
    while(coordinatesArray[counter%coordinatesMaxAmount][1] != 0 && coordinatesArray[counter%coordinatesMaxAmount][0] != 0 )
    {
          Serial.println("User is travelling");
          while (true) {
            if(sim808.getGPS()){
              Serial.println("Gps acquired");
              break;}
            }
          Serial.print("gpslatitude :");
          gpsLatitude = sim808.GPSdata.lat; // poziomo N/S
          Serial.print(gpsLatitude,7);
          Serial.print(" gpslongitude :");
          gpsLongitude = sim808.GPSdata.lon; //pionowo W/E
          Serial.println(gpsLongitude,7);

          sim808.detachGPS();
          Serial.print(" targetDeclination: ");
          targetDeclination = atan2(targetLongitude - gpsLongitude,targetLatitude - gpsLatitude) * ( 180.0 / M_PI );
          Serial.println(targetDeclination,7);

          while(compassCounter < 65530 ){
            if(compassCounter%2000==0){
              qmc.read(&x, &y, &z);
              userDeclination = atan2(x, y) * ( 180.0 / M_PI );
//              Serial.print(x);
//              Serial.print(" ");
//              Serial.print(y);
//              Serial.print(" ");
//              Serial.print(z);
//              Serial.print(" h ");
//              Serial.print(userDeclination);
//              Serial.print(" ");
              if(x==0 && y==0 && z==0 || x==-1 && y==-1 && z==-1){
                initialize_qmc();
                }
              //create logic for aiming user to correct direction
              if( userDeclination > -22.5 && userDeclination <= 22.5){Serial.print("N" );}
              if( userDeclination > 22.5 && userDeclination <= 67.5){Serial.print("NW ");}
              if( userDeclination > 67.5 && userDeclination <= 112.5){Serial.print("W ");}
              if( userDeclination > 112.5 && userDeclination <= 157.5){Serial.print("SW ");}
              if( userDeclination <= -157.5 || userDeclination > 157.5){Serial.print("S ");}
              if( userDeclination <= -112.5 && userDeclination > -157.5){Serial.print("SE ");}
              if( userDeclination <= -67.5 && userDeclination > -112.5){Serial.print("E ");}
              if( userDeclination <= -22.5 && userDeclination > -67.5){Serial.print("NE ");}

              Serial.print(targetDeclination - userDeclination);
              Serial.print(" ");
              Serial.print(targetDeclination);
              Serial.print(" ");
              Serial.println(userDeclination);
              if( abs(targetDeclination - userDeclination) >= abs(targetDeclination - userDeclination - 360) && abs(targetDeclination - userDeclination + 360) >= abs(targetDeclination - userDeclination - 360)){
                Serial.println("Left engine vibrates");
                //Left engine vibrates
              }
              if( abs(targetDeclination - userDeclination) >= abs(targetDeclination - userDeclination + 360) && abs(targetDeclination - userDeclination - 360) >= abs(targetDeclination - userDeclination + 360) ){
                Serial.println("right engine vibrates");
                // right engine vibrates
              }
              if( abs(targetDeclination - userDeclination) <= abs(targetDeclination - userDeclination - 360) && abs(targetDeclination - userDeclination) <= abs(targetDeclination - userDeclination + 360) && targetDeclination - userDeclination > 0){
                // right engine vibrates 
                Serial.println("right engine vibrates");
              }
              if( abs(targetDeclination - userDeclination) <= abs(targetDeclination - userDeclination - 360) && abs(targetDeclination - userDeclination) <= abs(targetDeclination - userDeclination + 360) && targetDeclination - userDeclination <0){
                Serial.println("Left engine vibrates");
                // left engine vibrates
              }
            } 
            compassCounter++;
          }
          compassCounter=0;
          if( fabs(targetLongitude - gpsLongitude) < 0.0002 && fabs(targetLatitude - gpsLatitude) < 0.0002 ){
            Serial.println("Checkpoint reached, updating...");
            coordinatesArray[counter][1] = 0;
            coordinatesArray[counter][0] = 0;
            // send coordinates' counter that will be set to 0 in server
            counter++;
            if(counter==coordinatesMaxAmount){counter=0;}
            targetLongitude = coordinatesArray[counter][1];
            targetLatitude = coordinatesArray[counter][0];
          }
      }
      Serial.println("End of route, turning off");
}
