# micro:bit ESP32 WiFi UART

micro:bit의 P1, P2 UART를 이용해 ESP32와 양방향 통신하는 MakeCode 확장입니다.

## 주요 기능

- P1 TX, P2 RX, 9600bps UART 시작
- ESP32로 일반 문자열 전송
- ESP32에 SSID와 비밀번호 전달
- ESP32 Wi-Fi 연결 명령 전송
- 서버에서 전달된 문자열 수신 이벤트
- 마지막으로 받은 서버 문자열 반환
- 수신 문자열 LED 표시

## 배선

| micro:bit | ESP32 DevKit V1 |
|---|---|
| P1 TX | GPIO16 RX2 |
| P2 RX | GPIO17 TX2 |
| GND | GND |

## MakeCode 사용 예

```typescript
esp32wifiuart.start()

esp32wifiuart.onServerMessage(function () {
    basic.showString(
        esp32wifiuart.serverMessage()
    )
})
```

Wi-Fi 설정:

```typescript
esp32wifiuart.setupWiFi(
    "aicampus_286",
    "aicampus286!!"
)
```

서버로 문자열 보내기:

```typescript
esp32wifiuart.sendLine("Hello")
```

## GitHub에서 MakeCode 확장으로 추가

1. 이 프로젝트를 GitHub 공개 저장소에 업로드합니다.
2. MakeCode micro:bit에서 새 프로젝트를 만듭니다.
3. `확장`을 선택합니다.
4. 검색창에 GitHub 저장소 주소를 입력합니다.

예:

```text
https://github.com/사용자명/microbit-esp32-wifi-uart
```

## 새 버전 반영

`main.ts` 수정 후 GitHub에 커밋한 다음 릴리스 태그를 생성합니다.

예:

```text
v0.0.1
v0.0.2
```

특정 버전을 직접 불러오려면:

```text
https://github.com/사용자명/microbit-esp32-wifi-uart#v0.0.2
```

## Supported targets

* for PXT/microbit
