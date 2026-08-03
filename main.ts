/**
 * micro:bit P1/P2 UART를 이용해 ESP32와 통신하는 확장
 */
//% color=#1565c0 weight=100 icon="\uf1eb" block="ESP32 WiFi UART"
namespace esp32wifiuart {

    let lastServerMessage = ""
    let started = false

    /**
     * P1을 TX, P2를 RX로 설정하고 9600bps UART를 시작합니다.
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

        serial.setRxBufferSize(64)
        basic.pause(100)
        started = true
    }

    /**
     * ESP32를 통해 서버로 문자열을 보냅니다.
     * 문자열 끝에 줄바꿈을 자동으로 추가합니다.
     *
     * @param text 보낼 문자열
     */
    //% blockId=esp32wifiuart_send_line
    //% block="서버로 문자열 보내기 %text"
    //% text.defl="Hello"
    //% weight=90
    export function sendLine(text: string): void {
        serial.writeLine(text)
    }

    /**
     * ESP32에 Wi-Fi SSID를 전달합니다.
     *
     * @param ssid Wi-Fi 이름
     */
    //% blockId=esp32wifiuart_set_ssid
    //% block="WiFi SSID 설정 %ssid"
    //% ssid.defl="aicampus_286"
    //% weight=80
    export function setSSID(ssid: string): void {
        serial.writeLine("SSID:" + ssid)
        basic.pause(150)
    }

    /**
     * ESP32에 Wi-Fi 비밀번호를 전달합니다.
     *
     * @param password Wi-Fi 비밀번호
     */
    //% blockId=esp32wifiuart_set_password
    //% block="WiFi 비밀번호 설정 %password"
    //% password.defl="password"
    //% weight=70
    export function setPassword(password: string): void {
        serial.writeLine("PASSWORD:" + password)
        basic.pause(150)
    }

    /**
     * ESP32에 Wi-Fi 연결 명령을 보냅니다.
     */
    //% blockId=esp32wifiuart_connect_wifi
    //% block="WiFi 연결"
    //% weight=60
    export function connectWiFi(): void {
        serial.writeLine("CONNECT")
    }

    /**
     * SSID와 비밀번호를 전달한 뒤 Wi-Fi 연결 명령을 보냅니다.
     *
     * @param ssid Wi-Fi 이름
     * @param password Wi-Fi 비밀번호
     */
    //% blockId=esp32wifiuart_setup_wifi
    //% block="WiFi 연결 SSID %ssid 비밀번호 %password"
    //% ssid.defl="aicampus_286"
    //% password.defl="password"
    //% weight=85
    export function setupWiFi(ssid: string, password: string): void {
        serial.writeLine("SSID:" + ssid)
        basic.pause(200)

        serial.writeLine("PASSWORD:" + password)
        basic.pause(200)

        serial.writeLine("CONNECT")
    }

    /**
     * ESP32에서 서버 문자열을 받으면 실행됩니다.
     */
    //% blockId=esp32wifiuart_on_server_message
    //% block="서버 문자열을 받았을 때"
    //% weight=50
    export function onServerMessage(handler: () => void): void {
        serial.onDataReceived(
            serial.delimiters(Delimiters.NewLine),
            function () {
                let message = serial.readUntil(
                    serial.delimiters(Delimiters.NewLine)
                )

                message = message.trim()

                if (message.length > 0) {
                    lastServerMessage = message
                    handler()
                }
            }
        )
    }

    /**
     * 마지막으로 받은 서버 문자열을 반환합니다.
     */
    //% blockId=esp32wifiuart_server_message
    //% block="받은 서버 문자열"
    //% weight=45
    export function serverMessage(): string {
        return lastServerMessage
    }

    /**
     * 마지막으로 받은 서버 문자열을 LED에 표시합니다.
     */
    //% blockId=esp32wifiuart_show_server_message
    //% block="받은 서버 문자열 LED 표시"
    //% weight=40
    export function showServerMessage(): void {
        if (lastServerMessage.length > 0) {
            basic.showString(lastServerMessage)
        }
    }

    /**
     * 마지막으로 받은 서버 문자열을 지웁니다.
     */
    //% blockId=esp32wifiuart_clear_server_message
    //% block="받은 서버 문자열 지우기"
    //% weight=30
    export function clearServerMessage(): void {
        lastServerMessage = ""
    }

    /**
     * ESP32 Wi-Fi 설정 삭제 명령을 보냅니다.
     */
    //% blockId=esp32wifiuart_clear_wifi
    //% block="ESP32 WiFi 설정 삭제"
    //% weight=20
    export function clearWiFi(): void {
        serial.writeLine("CLEAR")
    }
}
