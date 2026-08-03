#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>

// ==================================================
// Python 서버 설정
// ==================================================

// Flask 및 TCP 서버가 실행되는 PC의 IP
const char* SERVER_IP = "192.168.0.81";

// 마이크로비트 데이터를 서버로 보내는 HTTP 주소
const char* RECEIVE_URL =
    "http://192.168.0.81:5000/receive";

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

// ==================================================
// 재접속 시간
// ==================================================

unsigned long lastWiFiReconnect = 0;
unsigned long lastServerReconnect = 0;

const unsigned long WIFI_RECONNECT_INTERVAL = 5000;
const unsigned long SERVER_RECONNECT_INTERVAL = 3000;

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
    }
    else
    {
        Serial.println(
            "No saved WiFi settings"
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
        return false;
    }

    commandClient.stop();

    WiFi.disconnect(true);
    delay(300);

    WiFi.mode(WIFI_STA);

    Serial.print("Connecting to WiFi: ");
    Serial.println(wifiSSID);

    WiFi.begin(
        wifiSSID.c_str(),
        wifiPassword.c_str()
    );

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        if (millis() - startTime > 20000)
        {
            Serial.println();
            Serial.println(
                "WiFi connection failed"
            );

            return false;
        }
    }

    Serial.println();
    Serial.println("WiFi connected");

    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    return true;
}

// ==================================================
// 명령용 TCP 서버 연결
// ==================================================

void connectCommandServer()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    if (commandClient.connected())
    {
        return;
    }

    commandClient.stop();

    Serial.print(
        "Connecting command server: "
    );
    Serial.print(SERVER_IP);
    Serial.print(":");
    Serial.println(COMMAND_PORT);

    if (
        commandClient.connect(
            SERVER_IP,
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
}

// ==================================================
// 마이크로비트 데이터 HTTP 전송
// ==================================================

void sendToServer(String data)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi disconnected");
        return;
    }

    WiFiClient httpClient;
    HTTPClient http;

    if (!http.begin(httpClient, RECEIVE_URL))
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

        return;
    }

    // 저장 후 Wi-Fi 연결
    if (command == "CONNECT")
    {
        saveWiFi();

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

        wifiSSID = "";
        wifiPassword = "";

        commandClient.stop();
        WiFi.disconnect(true);

        Serial.println(
            "WiFi settings cleared"
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
}

// ==================================================
// setup
// ==================================================

void setup()
{
    // PC 디버깅
    Serial.begin(115200);

    // 마이크로비트 UART
    Serial2.begin(
        MICROBIT_BAUD,
        SERIAL_8N1,
        RX2_PIN,
        TX2_PIN
    );

    Serial2.setTimeout(100);

    Serial.println();
    Serial.println("ESP32 started");

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