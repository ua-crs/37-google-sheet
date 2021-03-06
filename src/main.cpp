/*
 *  Project 37-google-sheet - main.cpp
 *      Transferir cada 15 minutos
 *      la temperatura and humedad a
 *      Google Sheets
 *      Hecho a tarves de IFTTT y una cuenta de gmail
 */

#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include <DHT.h>

#include "wifi_ruts.h"

/*
 *  Definiciones a través de platformio.ini
 *      DHTPIN          IOPort donde está conectado data de DHTxx
 *      DHTTYPE         Tipo de sensor DHTxx - 11, 21 o 22
 *      SERIAL_BAUD     baud rate del port serie
 *      MY_SSID         SSID del WiFi
 *      MY_PASS         Password del WiFi
 *      RESOURCE        Recurso para direccionar GSheets a traves de IFTTT
 *      MINUTES_SLEEP   Tiempo en segundos entre mediciones
 */

/*
 *  Otras constantes
 */

const char *resource = RESOURCE;

// Maker Webhooks IFTTT
const char *server = "maker.ifttt.com";

const uint64_t SLEEP_MICROS = (MINUTES_SLEEP*60*1000000);

// Para esperar respuesta de IFTTT
const unsigned CLIENT_TOUT_SECS = 5;
const unsigned CLIENT_TEST_MSECS = 100;

/*
 *  Creación de objetos
 */

DHT dht(DHTPIN, DHTTYPE);       // Inicializacion sensor DHT

/*
 *  Global variables
 */

char tempC[20], tempF[20], hum[20];
WiFiClient client;


//  -------------- auxiliary functions ----------

static int
get_temphum( void )
{
    float h,t,f;

    h = dht.readHumidity();
    t = dht.readTemperature();              // en grados Celsius
    f = dht.readTemperature(true);          // en grados Fahrenheit

    if( isnan(h) || isnan(t) || isnan(f) )  //    verificar si cualquiera de las lecturas fracasaron
    {
        Serial.println(F("Error de lectura del sensor DHT"));
        return 0;
    }
	else
	{
		dtostrf( t, 3, 1, tempC );
		Serial.printf( "Temperatura = %s C\n\r", tempC );
		dtostrf( f, 3, 1, tempF );
		Serial.printf( "Temperatura = %s F\n\r", tempF );
		dtostrf( h, 3, 1, hum ); 
		Serial.printf( "Humedad = %s %%\n\r", hum );
		return 1;
	}
}

/*
 *  send_json:
 */

static void
send_json( void )
{
	String jsonObject; 

    jsonObject = String("{\"value1\":\"") + hum + "\",\"value2\":\"" + tempC + "\",\"value3\":\"" + tempF + "\"}";
    client.println(String("POST ") + resource + " HTTP/1.1");
    client.println(String("Host: ") + server);
    client.println("Connection: close\r\nContent-Type: application/json");
    client.print("Content-Length: ");
    client.println(jsonObject.length());
    client.println();
    client.println(jsonObject);
}

	
/*
 *  Pedido al servicio de Web de IFTTT
 */

static void
makeIFTTTRequest( void )
{
    unsigned timeout;

    Serial.printf( "Connecting to %s\n\r", server );

    for( unsigned retries = 5; !client.connect(server, 80) && retries--;  )
        Serial.print(".");
    Serial.println();

    if( !client.connected() )
    {
        Serial.println("Failed to connect...");
		return;
    }

    Serial.printf( "Request resource: %s\n\r", resource );

	if( !get_temphum() )
		return;

    send_json();

	for( timeout = CLIENT_TOUT_SECS * 1000 / CLIENT_TEST_MSECS; !client.available() && timeout--; )
        delay(CLIENT_TEST_MSECS);

    if( !client.available() )
        Serial.println("No response...");

    while( client.available() )
        Serial.write(client.read());

    Serial.println("\n\rclosing connection");
    client.stop();
}

//  -------------   main functions --------------

void
setup( void )
{
    Serial.begin(SERIAL_BAUD);
    dht.begin();

    connect_wifi(MY_SSID, MY_PASS);
	delay(500);
    makeIFTTTRequest();

    // start deep sleep for MINUTES_SLEEP minutes
    Serial.println("Going to sleep now");
    // enable timer deep sleep

    ESP.deepSleep(SLEEP_MICROS);

}

void
loop( void )
{
}
