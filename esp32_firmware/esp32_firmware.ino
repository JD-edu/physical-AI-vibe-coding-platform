#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==================================================
// OLED 설정
// SSD1306 I2C 128x64 OLED 기준
// ==================================================

const int OLED_SDA_PIN = 21;
const int OLED_SCL_PIN = 22;

const int OLED_WIDTH = 128;
const int OLED_HEIGHT = 64;

const int OLED_RESET_PIN = -1;
const uint8_t OLED_ADDRESS = 0x3C;

Adafruit_SSD1306 display(
    OLED_WIDTH,
    OLED_HEIGHT,
    &Wire,
    OLED_RESET_PIN
);

bool oledAvailable = false;

// OLED에 마지막으로 표시한 상태
wl_status_t lastDisplayedWiFiStatus = WL_NO_SHIELD;
bool lastDisplayedServerStatus = false;
String lastDisplayedSSID = "";
String lastDisplayedIP = "";

// ==================================================
// Python 서버 설정
// ==================================================

// Flask 및 TCP 서버가 실행되는 PC의 기본 IP
// 마이크로비트에서 SERVER_IP:xxx.xxx.xxx.xxx 명령으로 변경 가능
const char* DEFAULT_SERVER_IP = "192.168.0.81";

// 마이크로비트 데이터를 보내는 HTTP 포트와 경로
const int HTTP_PORT = 5000;
const char* RECEIVE_PATH = "/receive";

// 서버 명령 수신용 TCP 포트
const int COMMAND_PORT = 5001;

// ==================================================
// Serial2 설정
// ==================================================

const int RX2_PIN = 16;
const int TX2_PIN = 17;

const int MICROBIT_BAUD = 9600;

// ==================================================
// 객체
// ==================================================

Preferences preferences;

// 서버 명령 수신용 TCP 클라이언트
WiFiClient commandClient;

// ==================================================
// Wi-Fi 정보
// ==================================================

String wifiSSID = "";
String wifiPassword = "";
String serverIP = DEFAULT_SERVER_IP;

// ==================================================
// 재접속 시간
// ==================================================

unsigned long lastWiFiReconnect = 0;
unsigned long lastServerReconnect = 0;

const unsigned long WIFI_RECONNECT_INTERVAL = 5000;
const unsigned long SERVER_RECONNECT_INTERVAL = 3000;

// ==================================================
// OLED 문자열 길이 제한
// ==================================================

String shortenText(const String& text, int maxLength)
{
    if (text.length() <= maxLength)
    {
        return text;
    }

    if (maxLength <= 3)
    {
        return text.substring(0, maxLength);
    }

    return text.substring(0, maxLength - 3) + "...";
}

// ==================================================
// OLED 기본 메시지 표시
// ==================================================

void showOLEDMessage(
    const String& line1,
    const String& line2 = "",
    const String& line3 = "",
    const String& line4 = ""
)
{
    if (!oledAvailable)
    {
        return;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextWrap(false);

    display.setCursor(0, 0);
    display.println(shortenText(line1, 21));

    display.setCursor(0, 16);
    display.println(shortenText(line2, 21));

    display.setCursor(0, 32);
    display.println(shortenText(line3, 21));

    display.setCursor(0, 48);
    display.println(shortenText(line4, 21));

    display.display();
}

// ==================================================
// OLED 네트워크 상태 표시
// ==================================================

void updateOLEDStatus(bool forceUpdate = false)
{
    if (!oledAvailable)
    {
        return;
    }

    wl_status_t currentWiFiStatus = WiFi.status();
    bool currentServerStatus = commandClient.connected();

    String currentSSID;
    String currentIP;

    if (currentWiFiStatus == WL_CONNECTED)
    {
        currentSSID = WiFi.SSID();
        currentIP = WiFi.localIP().toString();
    }
    else
    {
        currentSSID = wifiSSID;
        currentIP = "Not connected";
    }

    // 상태가 바뀌지 않았다면 OLED를 다시 그리지 않음
    if (
        !forceUpdate &&
        currentWiFiStatus == lastDisplayedWiFiStatus &&
        currentServerStatus == lastDisplayedServerStatus &&
        currentSSID == lastDisplayedSSID &&
        currentIP == lastDisplayedIP
    )
    {
        return;
    }

    lastDisplayedWiFiStatus = currentWiFiStatus;
    lastDisplayedServerStatus = currentServerStatus;
    lastDisplayedSSID = currentSSID;
    lastDisplayedIP = currentIP;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextWrap(false);

    // 첫 번째 줄
    display.setCursor(0, 0);

    if (currentWiFiStatus == WL_CONNECTED)
    {
        display.println("WiFi: CONNECTED");
    }
    else
    {
        display.println("WiFi: DISCONNECTED");
    }

    // SSID 표시
    display.setCursor(0, 16);
    display.print("SSID:");
    display.println(
        shortenText(currentSSID, 15)
    );

    // IP 표시
    display.setCursor(0, 32);
    display.print("IP:");
    display.println(
        shortenText(currentIP, 18)
    );

    // TCP 서버 상태
    display.setCursor(0, 48);
    display.print("SERVER:");

    if (currentServerStatus)
    {
        display.println("CONNECTED");
    }
    else
    {
        display.println("OFFLINE");
    }

    display.display();
}

// ==================================================
// OLED 초기화
// ==================================================

void initializeOLED()
{
    Wire.begin(
        OLED_SDA_PIN,
        OLED_SCL_PIN
    );

    if (
        !display.begin(
            SSD1306_SWITCHCAPVCC,
            OLED_ADDRESS
        )
    )
    {
        oledAvailable = false;

        Serial.println(
            "SSD1306 OLED initialization failed"
        );

        return;
    }

    oledAvailable = true;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextWrap(false);

    display.setCursor(0, 0);
    display.println("ESP32 WiFi Bridge");

    display.setCursor(0, 16);
    display.println("OLED initialized");

    display.setCursor(0, 32);
    display.println("Starting...");

    display.display();

    Serial.println(
        "SSD1306 OLED initialized"
    );
}

// ==================================================
// 서버 IP 유효성 확인
// ==================================================

bool isValidServerIP(const String& ipText)
{
    IPAddress parsedIP;
    return parsedIP.fromString(ipText);
}

// ==================================================
// 서버 IP 저장
// ==================================================

void saveServerIP()
{
    preferences.begin("server", false);
    preferences.putString("ip", serverIP);
    preferences.end();

    Serial.print("Server IP saved: ");
    Serial.println(serverIP);
}

// ==================================================
// 서버 IP 읽기
// ==================================================

void loadServerIP()
{
    preferences.begin("server", true);
    serverIP = preferences.getString("ip", DEFAULT_SERVER_IP);
    preferences.end();

    if (!isValidServerIP(serverIP))
    {
        serverIP = DEFAULT_SERVER_IP;
    }

    Serial.print("Server IP: ");
    Serial.println(serverIP);
}

// ==================================================
// Wi-Fi 설정 저장
// ==================================================

void saveWiFi()
{
    preferences.begin("wifi", false);

    preferences.putString(
        "ssid",
        wifiSSID
    );

    preferences.putString(
        "password",
        wifiPassword
    );

    preferences.end();

    Serial.println("WiFi settings saved");

    showOLEDMessage(
        "WiFi settings",
        "saved",
        "SSID:",
        wifiSSID
    );
}

// ==================================================
// Wi-Fi 설정 읽기
// ==================================================

void loadWiFi()
{
    preferences.begin("wifi", true);

    wifiSSID =
        preferences.getString("ssid", "");

    wifiPassword =
        preferences.getString(
            "password",
            ""
        );

    preferences.end();

    if (wifiSSID.length() > 0)
    {
        Serial.print("Saved SSID: ");
        Serial.println(wifiSSID);

        showOLEDMessage(
            "Saved WiFi found",
            "SSID:",
            wifiSSID,
            "Connecting..."
        );
    }
    else
    {
        Serial.println(
            "No saved WiFi settings"
        );

        showOLEDMessage(
            "No WiFi settings",
            "Waiting for",
            "SSID/PASSWORD",
            "from microbit"
        );
    }
}

// ==================================================
// Wi-Fi 연결
// ==================================================

bool connectWiFi()
{
    if (wifiSSID.length() == 0)
    {
        Serial.println("SSID is empty");

        showOLEDMessage(
            "WiFi error",
            "SSID is empty"
        );

        return false;
    }

    commandClient.stop();

    WiFi.disconnect(true);
    delay(300);

    WiFi.mode(WIFI_STA);

    Serial.print("Connecting to WiFi: ");
    Serial.println(wifiSSID);

    showOLEDMessage(
        "Connecting WiFi",
        "SSID:",
        wifiSSID,
        "Please wait..."
    );

    WiFi.begin(
        wifiSSID.c_str(),
        wifiPassword.c_str()
    );

    unsigned long startTime = millis();
    int dotCount = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        dotCount++;

        if (dotCount > 10)
        {
            dotCount = 0;
        }

        if (oledAvailable)
        {
            display.clearDisplay();
            display.setTextColor(SSD1306_WHITE);
            display.setTextSize(1);
            display.setTextWrap(false);

            display.setCursor(0, 0);
            display.println("Connecting WiFi");

            display.setCursor(0, 16);
            display.print("SSID:");
            display.println(
                shortenText(wifiSSID, 15)
            );

            display.setCursor(0, 32);

            for (int i = 0; i < dotCount; i++)
            {
                display.print(".");
            }

            display.display();
        }

        if (millis() - startTime > 20000)
        {
            Serial.println();
            Serial.println(
                "WiFi connection failed"
            );

            showOLEDMessage(
                "WiFi connection",
                "FAILED",
                "SSID:",
                wifiSSID
            );

            return false;
        }
    }

    Serial.println();
    Serial.println("WiFi connected");

    Serial.print("Connected SSID: ");
    Serial.println(WiFi.SSID());

    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    updateOLEDStatus(true);

    return true;
}

// ==================================================
// 명령용 TCP 서버 연결
// ==================================================

void connectCommandServer()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        updateOLEDStatus();
        return;
    }

    if (commandClient.connected())
    {
        updateOLEDStatus();
        return;
    }

    commandClient.stop();

    Serial.print(
        "Connecting command server: "
    );
    Serial.print(serverIP);
    Serial.print(":");
    Serial.println(COMMAND_PORT);

    if (
        commandClient.connect(
            serverIP.c_str(),
            COMMAND_PORT
        )
    )
    {
        Serial.println(
            "Command server connected"
        );

        // 서버 로그에서 ESP32를 구분하기 위한 메시지
        commandClient.println(
            "ESP32_CONNECTED"
        );
    }
    else
    {
        Serial.println(
            "Command server connection failed"
        );
    }

    updateOLEDStatus(true);
}

// ==================================================
// 마이크로비트 데이터 HTTP 전송
// ==================================================

void sendToServer(String data)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi disconnected");

        updateOLEDStatus(true);
        return;
    }

    WiFiClient httpClient;
    HTTPClient http;

    String receiveURL =
        "http://" + serverIP + ":" +
        String(HTTP_PORT) + RECEIVE_PATH;

    if (!http.begin(httpClient, receiveURL))
    {
        Serial.println("HTTP begin failed");
        return;
    }

    http.setConnectTimeout(3000);
    http.setTimeout(3000);

    http.addHeader(
        "Content-Type",
        "text/plain; charset=utf-8"
    );

    int responseCode = http.POST(data);

    Serial.print("Sent to server: ");
    Serial.println(data);

    Serial.print("HTTP response: ");
    Serial.println(responseCode);

    if (responseCode > 0)
    {
        Serial.print("Server response: ");
        Serial.println(http.getString());
    }
    else
    {
        Serial.print("HTTP error: ");

        Serial.println(
            HTTPClient::errorToString(
                responseCode
            )
        );
    }

    http.end();
}

// ==================================================
// 마이크로비트 명령 처리
// ==================================================

void processMicrobitCommand(String command)
{
    command.trim();

    if (command.length() == 0)
    {
        return;
    }

    Serial.print("Microbit received: [");
    Serial.print(command);
    Serial.println("]");

    // SSID 설정
    if (command.startsWith("SSID:"))
    {
        wifiSSID = command.substring(5);
        wifiSSID.trim();

        Serial.print("SSID received: ");
        Serial.println(wifiSSID);

        showOLEDMessage(
            "SSID received",
            wifiSSID,
            "Waiting for",
            "PASSWORD"
        );

        return;
    }

    // 비밀번호 설정
    if (command.startsWith("PASSWORD:"))
    {
        wifiPassword =
            command.substring(9);

        Serial.println(
            "Password received"
        );

        showOLEDMessage(
            "Password received",
            "Send CONNECT",
            "to start WiFi"
        );

        return;
    }

    // 서버 IP 설정 및 저장
    if (command.startsWith("SERVER_IP:"))
    {
        String newServerIP = command.substring(10);
        newServerIP.trim();

        if (!isValidServerIP(newServerIP))
        {
            Serial.print("Invalid server IP: ");
            Serial.println(newServerIP);
            Serial2.println("ERROR:INVALID_SERVER_IP");

            showOLEDMessage(
                "Server IP error",
                "Invalid address",
                newServerIP
            );

            return;
        }

        serverIP = newServerIP;
        saveServerIP();

        // 기존 서버 연결은 끊고 새 IP로 다시 연결
        commandClient.stop();
        lastServerReconnect = 0;

        Serial2.println("SERVER_IP_SAVED");

        showOLEDMessage(
            "Server IP saved",
            serverIP,
            "TCP port: 5001",
            "HTTP port: 5000"
        );

        if (WiFi.status() == WL_CONNECTED)
        {
            connectCommandServer();
        }

        return;
    }

    // 저장 후 Wi-Fi 연결
    if (command == "CONNECT")
    {
        saveWiFi();
        saveServerIP();

        if (connectWiFi())
        {
            connectCommandServer();
        }

        return;
    }

    // Wi-Fi 설정 삭제
    if (command == "CLEAR")
    {
        preferences.begin("wifi", false);
        preferences.clear();
        preferences.end();

        preferences.begin("server", false);
        preferences.clear();
        preferences.end();

        wifiSSID = "";
        wifiPassword = "";
        serverIP = DEFAULT_SERVER_IP;

        commandClient.stop();
        WiFi.disconnect(true);

        Serial.println(
            "WiFi settings cleared"
        );

        showOLEDMessage(
            "WiFi settings",
            "CLEARED",
            "Waiting for",
            "new settings"
        );

        return;
    }

    // 그 외 문자열은 Flask 서버로 전송
    sendToServer(command);
}

// ==================================================
// 마이크로비트 데이터 수신
// ==================================================

void checkMicrobitSerial()
{
    if (Serial2.available() <= 0)
    {
        return;
    }

    String command =
        Serial2.readStringUntil('\n');

    processMicrobitCommand(command);
}

// ==================================================
// Python 서버 명령 수신
// ==================================================

void checkServerCommand()
{
    if (!commandClient.connected())
    {
        return;
    }

    while (commandClient.available() > 0)
    {
        String command =
            commandClient.readStringUntil('\n');

        command.trim();

        if (command.length() == 0)
        {
            continue;
        }

        Serial.print("Server command: [");
        Serial.print(command);
        Serial.println("]");

        // 마이크로비트로 줄바꿈 포함 전송
        Serial2.println(command);

        Serial.println(
            "Command sent to microbit"
        );
    }
}

// ==================================================
// Wi-Fi 및 서버 재접속
// ==================================================

void maintainConnections()
{
    // Wi-Fi 재접속
    if (WiFi.status() != WL_CONNECTED)
    {
        updateOLEDStatus();

        if (
            wifiSSID.length() > 0 &&
            millis() - lastWiFiReconnect
                >= WIFI_RECONNECT_INTERVAL
        )
        {
            lastWiFiReconnect = millis();

            Serial.println(
                "WiFi reconnecting"
            );

            WiFi.disconnect();

            WiFi.begin(
                wifiSSID.c_str(),
                wifiPassword.c_str()
            );
        }

        return;
    }

    // TCP 명령 서버 재접속
    if (!commandClient.connected())
    {
        if (
            millis() - lastServerReconnect
                >= SERVER_RECONNECT_INTERVAL
        )
        {
            lastServerReconnect = millis();

            connectCommandServer();
        }
    }

    updateOLEDStatus();
}

// ==================================================
// setup
// ==================================================

void setup()
{
    // PC 디버깅
    Serial.begin(115200);

    delay(300);

    Serial.println();
    Serial.println("ESP32 started");

    // OLED 초기화
    initializeOLED();

    // 마이크로비트 UART
    Serial2.begin(
        MICROBIT_BAUD,
        SERIAL_8N1,
        RX2_PIN,
        TX2_PIN
    );

    Serial2.setTimeout(100);

    loadServerIP();
    loadWiFi();

    if (wifiSSID.length() > 0)
    {
        if (connectWiFi())
        {
            connectCommandServer();
        }
    }
    else
    {
        Serial.println(
            "Waiting for WiFi settings"
        );

        updateOLEDStatus(true);
    }
}

// ==================================================
// loop
// ==================================================

void loop()
{
    // 마이크로비트 → ESP32
    checkMicrobitSerial();

    // 서버 → ESP32 → 마이크로비트
    checkServerCommand();

    // 연결 상태 유지
    maintainConnections();

    delay(5);
}