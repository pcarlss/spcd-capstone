#include <WiFi.h>
#include <WebServer.h>

// Network credentials
const char *ssid = "SPCD";  // Your desired SSID (Wi-Fi network name)
const char *password = "admin2024";       // Your desired password (min 8 characters)

// Create an instance of the WebServer on port 80 (default HTTP port)
WebServer server(80);

// Function to handle requests to the root page
void handleRoot() {
  // HTML content to display
  String html = "<html>"
                "<head><title>SPCD: Live Feed</title></head>"
                "<body>"
                "<h1 style='color:black;'>SPCD</h1>"
                "<h2>Live Feed</h2>"
                "<div style='border:2px solid black; width:640px; height:360px;'>"
                "<p>Video Livestream Coming Soon</p>"
                "</div>"
                "</body></html>";


  // Send the HTML content with HTTP status 200 (OK)
  server.send(200, "text/html", html);
}

void setup() {
  // Start the serial communication for debugging
  Serial.begin(115200);

  // Start the Access Point with the given SSID and password
  WiFi.softAP(ssid, password, 1);

  // Print the IP address of the Access Point
  Serial.println("Access Point Created!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Define the handler for the root URL ("/")
  server.on("/", handleRoot);

  // Start the web server
  server.begin();
  Serial.println("HTTP server started");

  Serial.println(WiFi.softAPIP());
}

void loop() {
  // Handle any incoming client requests
  server.handleClient();
}