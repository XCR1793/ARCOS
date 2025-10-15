#include <Arduino.h>

#define GPS_BAUD 9600

// Create an instance of the HardwareSerial class for Serial 2
HardwareSerial gpsSerial(2);

// GPS Data Variables
String nmea_sentence = "";
double gps_latitude = 0.0;
double gps_longitude = 0.0;
uint8_t gps_satellites = 0;
uint8_t gps_fix_quality = 0;
float gps_altitude = 0.0;
float gps_speed_knots = 0.0;
String gps_time = "";
unsigned long last_update = 0;
int total_sentences = 0;

// Parse NMEA GPGGA/GNGGA sentence: $GPGGA or $GNGGA,time,lat,N/S,lon,E/W,fix,sats,hdop,alt,M,...
// Example: $GPGGA,135025.00,3343.61042,S,15055.10503,E,1,07,1.17,79.1,M,19.6,M,,*75
// Example: $GNGGA,110827.00,4114.32485,N,00831.79799,W,1,10,0.93,130.6,M,50.1,M,*5F
void parseGpgga(String sentence){
  int field_index = 0;
  int start_pos = 0;
  String fields[15];
  
  // Split by commas
  for(int i = 0; i < sentence.length(); i++){
    if(sentence[i] == ',' || sentence[i] == '*'){
      fields[field_index++] = sentence.substring(start_pos, i);
      start_pos = i + 1;
      if(field_index >= 15) break;
    }
  }
  
  if(field_index >= 10 && (fields[0] == "$GPGGA" || fields[0] == "$GNGGA")){
    // Field 1: Time (HHMMSS.SS)
    if(fields[1].length() >= 6){
      gps_time = fields[1].substring(0, 2) + ":" + 
                 fields[1].substring(2, 4) + ":" + 
                 fields[1].substring(4, 6);
    }
    
    // Field 6: Fix quality (0=none, 1=GPS, 2=DGPS)
    gps_fix_quality = fields[6].toInt();
    
    // Field 7: Number of satellites
    gps_satellites = fields[7].toInt();
    
    // Field 2-3: Latitude (DDMM.MMMMM format)
    if(fields[2].length() > 0 && fields[3].length() > 0){
      double lat_raw = fields[2].toDouble();
      int lat_degrees = (int)(lat_raw / 100);
      double lat_minutes = lat_raw - (lat_degrees * 100);
      gps_latitude = lat_degrees + (lat_minutes / 60.0);
      if(fields[3] == "S") gps_latitude = -gps_latitude;
    }
    
    // Field 4-5: Longitude (DDDMM.MMMMM format)
    if(fields[4].length() > 0 && fields[5].length() > 0){
      double lon_raw = fields[4].toDouble();
      int lon_degrees = (int)(lon_raw / 100);
      double lon_minutes = lon_raw - (lon_degrees * 100);
      gps_longitude = lon_degrees + (lon_minutes / 60.0);
      if(fields[5] == "W") gps_longitude = -gps_longitude;
    }
    
    // Field 9: Altitude in meters
    if(fields[9].length() > 0){
      gps_altitude = fields[9].toFloat();
    }
    
    last_update = millis();
  }
}

// Parse NMEA GPRMC/GNRMC sentence: $GPRMC or $GNRMC,time,status,lat,N/S,lon,E/W,speed,track,date,...
// Example: $GPRMC,135026.00,A,3343.61039,S,15055.10501,E,0.146,,151025,,,A*64
// Example: $GNRMC,110827.00,A,4114.32485,N,00831.79799,W,0.0,,date,,,A*XX
void parseGprmc(String sentence){
  int field_index = 0;
  int start_pos = 0;
  String fields[13];
  
  // Split by commas
  for(int i = 0; i < sentence.length(); i++){
    if(sentence[i] == ',' || sentence[i] == '*'){
      fields[field_index++] = sentence.substring(start_pos, i);
      start_pos = i + 1;
      if(field_index >= 13) break;
    }
  }
  
  if(field_index >= 8 && (fields[0] == "$GPRMC" || fields[0] == "$GNRMC")){
    // Field 7: Speed over ground in knots
    if(fields[7].length() > 0){
      gps_speed_knots = fields[7].toFloat();
    }
  }
}

void setup(){
  // Serial Monitor
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== GPS Test Program ===");
  Serial.println("Parsing NMEA sentences from NEO-M8 GPS");
  Serial.println("Pins: RX=GPIO44, TX=GPIO43, Baud=9600\n");
  
  // Start Serial 2 with the defined RX and TX pins and a baud rate of 9600
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, 44, 43);
  Serial.println("GPS Serial started!\n");
}

void loop(){
  // Process incoming GPS data
  while(gpsSerial.available() > 0){
    char c = gpsSerial.read();
    
    // Build NMEA sentence
    if(c == '$'){
      nmea_sentence = "$";  // Start new sentence
      total_sentences++;
    }else if(c == '\n'){
      // End of sentence - parse it
      if(nmea_sentence.startsWith("$GPGGA") || nmea_sentence.startsWith("$GNGGA")){
        parseGpgga(nmea_sentence);
      }else if(nmea_sentence.startsWith("$GPRMC") || nmea_sentence.startsWith("$GNRMC")){
        parseGprmc(nmea_sentence);
      }
      nmea_sentence = "";
    }else if(nmea_sentence.length() < 100){  // Prevent overflow
      nmea_sentence += c;
    }
  }
  
  // Print parsed GPS data every second
  static unsigned long last_print = 0;
  if(millis() - last_print >= 1000){
    last_print = millis();
    
    Serial.println("=================================");
    Serial.print("Time (UTC):    "); Serial.println(gps_time);
    Serial.print("Fix Quality:   "); 
    if(gps_fix_quality == 0) Serial.println("No Fix");
    else if(gps_fix_quality == 1) Serial.println("GPS Fix");
    else if(gps_fix_quality == 2) Serial.println("DGPS Fix");
    else Serial.println(gps_fix_quality);
    
    Serial.print("Satellites:    "); Serial.println(gps_satellites);
    Serial.print("Latitude:      "); Serial.print(gps_latitude, 6); Serial.println("°");
    Serial.print("Longitude:     "); Serial.print(gps_longitude, 6); Serial.println("°");
    Serial.print("Altitude:      "); Serial.print(gps_altitude, 1); Serial.println(" m");
    Serial.print("Speed:         "); Serial.print(gps_speed_knots, 2); Serial.print(" knots (");
    Serial.print(gps_speed_knots * 1.852, 2); Serial.println(" km/h)");
    Serial.print("Total NMEA:    "); Serial.println(total_sentences);
    Serial.print("Last Update:   "); Serial.print(millis() - last_update); Serial.println(" ms ago");
    Serial.println();
  }
}