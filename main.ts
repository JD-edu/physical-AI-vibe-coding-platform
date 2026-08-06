/**
 * micro:bit P1/P2 UART를 이용해 ESP32와 통신하는 확장
 */
//% color=#1565c0 weight=100 icon="\uf1eb" block="ESP32 WiFi UART"
namespace esp32wifiuart {

    let lastServerMessage = ""
    let lastESP32Status = ""
    let started = false

    /**
     * P1=TX, P2=RX, 9600bps로 UART를 시작하고
     * ESP32에 마이크로비트 시작 신호를 보냅니다.
     */
    //% blockId=esp32wifiuart_start
    //% block="ESP32 시리얼 시작"
    //% weight=100
    export function start(): void {
        serial.redirect(
            SerialPin.P1,
            SerialPin.P2,
            BaudRate.BaudRate9600
        )

        serial.setRxBufferSize(128)
        basic.pause(500)

        // ESP32가 부팅을 마치고 UART를 준비할 시간을 고려해 시작 신호 전송
        serial.writeLine("MB_START")
        started = true
    }

    /** ESP32에 마이크로비트 시작 신호를 다시 보냅니다. */
    //% blockId=esp32wifiuart_send_start_signal
    //% block="ESP32 시작 신호 보내기"
    //% weight=95
    export function sendStartSignal(): void {
        serial.writeLine("MB_START")
    }

    /** 서버로 문자열을 보냅니다. */
    //% blockId=esp32wifiuart_send_line
    //% block="서버로 문자열 보내기 %text"
    //% text.defl="Hello"
    //% weight=90
    export function sendLine(text: string): void {
        serial.writeLine(text)
    }

    /** ESP32에 Wi-Fi SSID를 전달합니다. */
    //% blockId=esp32wifiuart_set_ssid
    //% block="WiFi SSID 설정 %ssid"
    //% ssid.defl="aicampus_286"
    //% weight=80
    export function setSSID(ssid: string): void {
        serial.writeLine("SSID:" + ssid)
        basic.pause(200)
    }

    /** ESP32에 Wi-Fi 비밀번호를 전달합니다. */
    //% blockId=esp32wifiuart_set_password
    //% block="WiFi 비밀번호 설정 %password"
    //% password.defl="password"
    //% weight=70
    export function setPassword(password: string): void {
        serial.writeLine("PASSWORD:" + password)
        basic.pause(200)
    }

    /** ESP32가 접속할 서버 IP를 전달합니다. */
    //% blockId=esp32wifiuart_set_server_ip
    //% block="서버 IP 설정 %ip"
    //% ip.defl="192.168.0.81"
    //% weight=65
    export function setServerIP(ip: string): void {
        serial.writeLine("SERVER_IP:" + ip)
        basic.pause(200)
    }

    /** 전달된 설정으로 Wi-Fi와 서버 연결을 시작합니다. */
    //% blockId=esp32wifiuart_connect_wifi
    //% block="WiFi와 서버 연결 시작"
    //% weight=60
    export function connectWiFi(): void {
        serial.writeLine("CONNECT")
    }

    /** SSID와 비밀번호를 전달하고 연결합니다. */
    //% blockId=esp32wifiuart_setup_wifi
    //% block="WiFi 연결 SSID %ssid 비밀번호 %password"
    //% ssid.defl="aicampus_286"
    //% password.defl="password"
    //% weight=85
    export function setupWiFi(ssid: string, password: string): void {
        ensureStarted()
        setSSID(ssid)
        setPassword(password)
        connectWiFi()
    }

    /** SSID, 비밀번호, 서버 IP를 전달하고 연결합니다. */
    //% blockId=esp32wifiuart_setup_wifi_server
    //% block="WiFi와 서버 연결 SSID %ssid 비밀번호 %password 서버 IP %ip"
    //% ssid.defl="aicampus_286"
    //% password.defl="password"
    //% ip.defl="192.168.0.81"
    //% weight=84
    export function setupWiFiAndServer(ssid: string, password: string, ip: string): void {
        ensureStarted()
        setSSID(ssid)
        setPassword(password)
        setServerIP(ip)
        connectWiFi()
    }

    /** ESP32에서 서버 문자열을 받았을 때 실행됩니다. */
    //% blockId=esp32wifiuart_on_server_message
    //% block="서버 문자열을 받았을 때"
    //% weight=50
    export function onServerMessage(handler: () => void): void {
        serial.onDataReceived(
            serial.delimiters(Delimiters.NewLine),
            function () {
                let message = serial.readUntil(
                    serial.delimiters(Delimiters.NewLine)
                ).trim()

                if (message.length == 0) {
                    return
                }

                // ESP32 내부 상태 메시지는 서버 메시지와 분리
                if (message.indexOf("STATUS:") == 0) {
                    lastESP32Status = message.substr(7)
                } else {
                    lastServerMessage = message
                    handler()
                }
            }
        )
    }

    /** 마지막 서버 문자열을 반환합니다. */
    //% blockId=esp32wifiuart_server_message
    //% block="받은 서버 문자열"
    //% weight=45
    export function serverMessage(): string {
        return lastServerMessage
    }

    /** ESP32의 마지막 상태 문자열을 반환합니다. */
    //% blockId=esp32wifiuart_esp32_status
    //% block="ESP32 연결 상태"
    //% weight=44
    export function esp32Status(): string {
        return lastESP32Status
    }

    /** 마지막 서버 문자열을 LED에 표시합니다. */
    //% blockId=esp32wifiuart_show_server_message
    //% block="받은 서버 문자열 LED 표시"
    //% weight=40
    export function showServerMessage(): void {
        if (lastServerMessage.length > 0) {
            basic.showString(lastServerMessage)
        }
    }

    /** 마지막 서버 문자열을 지웁니다. */
    //% blockId=esp32wifiuart_clear_server_message
    //% block="받은 서버 문자열 지우기"
    //% weight=30
    export function clearServerMessage(): void {
        lastServerMessage = ""
    }

    /** ESP32의 Wi-Fi 및 서버 설정을 삭제합니다. */
    //% blockId=esp32wifiuart_clear_wifi
    //% block="ESP32 WiFi 설정 삭제"
    //% weight=20
    export function clearWiFi(): void {
        serial.writeLine("CLEAR")
    }

    function ensureStarted(): void {
        if (!started) {
            start()
            basic.pause(300)
        }
    }
}